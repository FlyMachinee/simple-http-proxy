#include <atomic>
#include <chrono>
#include <thread>

#include "../include/HttpProxyServer.h"
#include "../include/HttpRequest.h"
#include "../include/HttpResponseHead.h"
#include "../include/format_log.hpp"
#include "../include/util.h"
#include "../include/wsa_wapper.h"

int ::my::HttpProxyServer::instance_count_ = 0;
int ::my::HttpProxyServer::p_id_ = 0;

::std::atomic_bool keybord_interrupt = false;
BOOL WINAPI keybord_interrupt_handler(DWORD ctrl_type)
{
    if (ctrl_type == CTRL_C_EVENT) {
        keybord_interrupt.store(true);
        return TRUE;
    }
    return FALSE;
}

::my::HttpProxyServer::HttpProxyServer(const char *p_ip, unsigned short p_port, bool use_cache)
    : use_cache_(use_cache), cache_manager_(".\\cache"), task_count_(0)
{
    log("Initializing proxy<{}> ...", p_id_);

    proxy_.ip = p_ip;
    proxy_.port = p_port;

    try {
        if (instance_count_ == 0 && !wsa_initialized) {
            log("Detected Winsock uninitialized");
            if (!init_wsa()) {
                throw std::runtime_error("Failed to initialize Winsock");
            }
        }

        proxy_.socket = socket(AF_INET, SOCK_STREAM, 0);
        if (proxy_.socket == INVALID_SOCKET) {
            throw std::runtime_error(::std::format("Failed to create socket. Error code: {}", WSAGetLastError()));
        }
        log("Socket created");

        SOCKADDR_IN proxy_addr;
        proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_port = htons(p_port);
        proxy_addr.sin_addr.s_addr = inet_addr(p_ip);

        if (bind(proxy_.socket, reinterpret_cast<SOCKADDR *>(&proxy_addr), sizeof(proxy_addr)) == SOCKET_ERROR) {
            throw std::runtime_error(::std::format("Failed to bind socket. Error code: {}", WSAGetLastError()));
        }
        log("Socket bound to {}:{}", p_ip, p_port);

        if (listen(proxy_.socket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error(::std::format("Failed to listen on socket. Error code: {}", WSAGetLastError()));
        }
    } catch (const std::runtime_error &e) {
        if (proxy_.socket != INVALID_SOCKET) {
            closesocket(proxy_.socket);
        }
        err("In Proxy<{}> constructor:", instance_count_);
        con<8>("{}", e.what());
        throw e;
    }
    log("Proxy server will listen on {}:{}", p_ip, p_port);
    log("Proxy<{}> initialized\n", p_id_);
    instance_count_++;
    p_no_ = p_id_++;
}

::my::HttpProxyServer::~HttpProxyServer()
{
    if (proxy_.socket != INVALID_SOCKET) {
        closesocket(proxy_.socket);
        log("Socket closed");
    }

    log("Proxy<{}> destroyed.\n", p_no_);

    if (--instance_count_ == 0 && wsa_initialized) {
        if (!cleanup_wsa()) {
            err("In proxy<{}> destructor:", p_no_);
            con<8>("Failed to clean up Winsock");
        }
    }
}

bool ::my::HttpProxyServer::run()
{
    SetConsoleCtrlHandler(keybord_interrupt_handler, TRUE);

    log("Proxy server {} is running, listening on {}:{} ...\n", p_no_, proxy_.ip, proxy_.port);

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(proxy_.socket, &readfds);
    TIMEVAL timeout = {0, 0};

    Host client;
    int client_cnt = 0;

    while (!keybord_interrupt.load()) {
        while (!keybord_interrupt.load()) {
            fd_set readfds_copy = readfds;
            int sum = select(0, &readfds_copy, nullptr, nullptr, &timeout);
            if (sum == SOCKET_ERROR) {
                err("In Proxy<{}>:", p_no_);
                con<8>("Failed to call select. Error code: {}", WSAGetLastError());
                con<8>("Continue listening...");
            } else if (sum == 0) {
                ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
            } else {
                break;
            }
        }
        if (keybord_interrupt.load()) {
            break;
        }

        client.socket = accept(proxy_.socket, nullptr, nullptr);
        if (client.socket == INVALID_SOCKET) {
            err("In Proxy<{}>:", p_no_);
            con<8>("Failed to accept client. Error code: {}", WSAGetLastError());
            con<8>("Continue listening...");
            continue;
        }
        client.update();

        ++client_cnt;

        con<0>("==========================| task {} |=============================", client_cnt);
        ::std::thread(&::my::HttpProxyServer::handle_client, this, client_cnt, client).join();
        task_count_.fetch_add(1);
        con<0>("=======================| end of task {} |=========================\n", client_cnt);
    }

    SetConsoleCtrlHandler(keybord_interrupt_handler, FALSE);

    log("Keyboard interrupt detected, stopping proxy server {} ...", p_no_);
    con<6>("Waiting for all tasks to finish, task count: {}\n", task_count_.load());
    while (task_count_.load() > 0) {
        ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
    }

    log("Proxy server {} stopped\n", p_no_);

    return true;
}

void ::my::HttpProxyServer::handle_client(int c_no, Host client)
{
    // HOSTENT *s_hostent = nullptr;
    Host server;
    ::std::string s_hostname;
    int recv_cnt, recv_size, send_size;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(client.socket, &readfds);
    TIMEVAL timeout = {1, 0};

    try {

        log("Proxy<{}>: connected with client<{}>: {}:{}", p_no_, c_no, client.ip, client.port);
        con<6>("{}:{} ------------- {}:{} - - - - ?:?", client.ip, client.port, proxy_.ip, proxy_.port);

        char buffer[MAX_MSG_SIZE];
        if (select(0, &readfds, nullptr, nullptr, &timeout) == 0) {
            throw ::std::runtime_error(::std::format("Timeout(1s) when receiving data from client<{}>", c_no));
        }

        recv_size = recv(client.socket, buffer, MAX_MSG_SIZE, 0);
        if (recv_size == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to receive data from client<{}>. Error code: {}", c_no, WSAGetLastError()));
        } else if (recv_size == 0) {
            throw ::std::runtime_error(::std::format("Client<{}> disconnected", c_no));
        }

        HttpRequest c_req(buffer, recv_size);
        ::std::tie(s_hostname, server.port) = c_req.get_host_port();
        server.ip = get_ip_str(s_hostname.c_str());

        log("Proxy<{}>: received {} bytes data from client<{}> successfully:", p_no_, recv_size, c_no);
        // out_http_data(10, buffer, recv_size);
        con<6>("{}:{} ====[{}]===> {}:{} - - - - - - - {}:{} ({})", client.ip, client.port, c_req.method, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);
        con<6>("URL: {}", c_req.url);

        if (c_req.method == "GET" || c_req.method == "POST") {
            try {
                server.socket = connect_to_server(server.ip.c_str(), server.port);
            } catch (const ::std::runtime_error &e) {
                throw ::std::runtime_error(::std::format("During connecting to server {}:{}", server.ip, server.port) + "\n        " + e.what());
            }

            log("Proxy<{}>: enstabished connection with server {}", p_no_, s_hostname);
            con<6>("{}:{} ------------- {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);

            // ::std::cout << "DEBUG: about to check cache" << ::std::endl;
            if (check_cache(c_req, server)) {
                log("Proxy<{}>: cache hit for: {} ({})", p_no_, c_req.url, HttpCacheManager::get_key(c_req.url));

                int total_size = answer_from_cache(c_req.url, client);

                log("Proxy<{}>: transmitted {} bytes data from cache to client<{}> successfully", p_no_, total_size, c_no);
                con<6>("{}:{} <==[cached]== {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);

            } else {
                ::std::cout << "DEBUG: about to answer from server" << ::std::endl;

                ::std::string status;
                int total_size = answer_from_server(c_req, client, server, status);
                if (total_size == 0) {
                    throw ::std::runtime_error("Server disconnected");
                }

                log("Proxy<{}>: transmitted {} bytes data from server {} to client<{}> successfully", p_no_, total_size, s_hostname, c_no);
                con<6>("{}:{} <================[ {} ]================= {}:{} ({})", client.ip, client.port, status, server.ip, server.port, s_hostname);
            }
        } else {
            send(client.socket, "HTTP/1.1 405 Method Not Allowed\r\n\r\n", 34, 0);
            log("Proxy<{}>: received an unsupported method {} from client<{}>, rejected", p_no_, c_req.method, c_no);
            con<6>("{}:{} <====[ 405 ]==== {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);
        }
    } catch (const ::std::exception &e) {
        err("In Proxy<{}>:", p_no_);
        con<8>("{}", e.what());
        // throw e;
    }

    if (server.socket != INVALID_SOCKET) {
        closesocket(server.socket);
        log("Proxy<{}>: disconnected from server {}", p_no_, s_hostname);
    }
    closesocket(client.socket);
    log("Proxy<{}>: disconnected from client<{}>", p_no_, c_no);

    task_count_.fetch_sub(1);
}

bool my::HttpProxyServer::check_cache(const HttpRequest &c_req, const Host &server)
{
    if (!use_cache_ || c_req.method != "GET" || !cache_manager_.has_cache(c_req.url)) {
        return false;
    }

    // ::std::cout << "DEBUG: about to check time for: " << c_req.url << '(' << HttpCacheManager::get_key(c_req.url) << ')' << ::std::endl;

    ::std::string check_request =
        "GET " + c_req.url + " HTTP/1.1\r\nHost: " +
        c_req.headers.at("Host") + "\r\n";

    if (cache_manager_.get_modified_time(c_req.url) != "") {
        check_request += "If-Modified-Since: " + cache_manager_.get_modified_time(c_req.url) + "\r\n";
    }
    if (cache_manager_.get_etag(c_req.url) != "") {
        check_request += "If-None-Match: " + cache_manager_.get_etag(c_req.url) + "\r\n";
    }
    check_request += "\r\n";

    // ::std::cout << "DEBUG: check request: " << check_request << ::std::endl;

    if (send(server.socket, check_request.c_str(), check_request.length(), 0) == SOCKET_ERROR) {
        throw ::std::runtime_error(::std::format("Failed to send check request to server. Error code: {}", WSAGetLastError()));
    }

    char buffer[1024];
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server.socket, &readfds);
    TIMEVAL timeout = {1, 0};

    if (select(0, &readfds, nullptr, nullptr, &timeout) == 0) {
        throw ::std::runtime_error("Timeout(1s) when receiving check result from server");
    }
    int recv_size = recv(server.socket, buffer, 1024, 0);
    if (recv_size == SOCKET_ERROR) {
        throw ::std::runtime_error(::std::format("Failed to receive check result from server. Error code: {}", WSAGetLastError()));
    }

    const char *start = strchr(buffer, ' ') + 1;
    const char *end = ::std::min(strchr(start, ' '), strchr(start, '\r'));
    ::std::string status = ::std::string(start, end - start);

    ::std::cout << "DEBUG: check response: " << status << ::std::endl;
    if (status == "400") {
        throw ::std::runtime_error("Bad request received from server when checking cache");
    }
    return status == "304";
}

int my::HttpProxyServer::answer_from_cache(::std::string_view url, const Host &client)
{
    char buffer[MAX_MSG_SIZE];
    int total_size = 0;
    int read_size;
    int pkg_cnt = 0;
    while ((read_size = cache_manager_.read_cache(url, buffer, MAX_MSG_SIZE, total_size)) > 0) {
        if (send(client.socket, buffer, read_size, 0) == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to send data (pack {}, {} bytes) to client. Error code: {}", pkg_cnt, read_size, WSAGetLastError()));
        }
        total_size += read_size;
        ++pkg_cnt;
    }
    return total_size;
}

int my::HttpProxyServer::answer_from_server(HttpRequest c_req, const Host &client, const Host &server, ::std::string &status)
{
    char buffer[MAX_MSG_SIZE];
    int recv_size;

    ::std::cout << "DEBUG: about to send GET/POST request to server" << ::std::endl;
    c_req.headers.erase("Proxy-Connection");
    c_req.headers["Connection"] = "close";
    ::std::string request_buffer = c_req.to_string();
    if (send(server.socket, request_buffer.c_str(), request_buffer.length(), 0) == SOCKET_ERROR) {
        throw ::std::runtime_error(::std::format("Failed to send request to server. Error code: {}", WSAGetLastError()));
    }

    int total_size = 0;
    bool is_first = true;
    bool need_cache = use_cache_ && c_req.method == "GET";
    int pkg_cnt = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server.socket, &readfds);
    TIMEVAL timeout = {1, 0};

    while (true) {
        ::std::cout << "DEBUG: about to receive data from server: pack " << pkg_cnt << ::std::endl;

        fd_set readfds_copy = readfds;
        if (select(0, &readfds_copy, nullptr, nullptr, &timeout) == 0) {
            throw ::std::runtime_error(::std::format("Timeout(1s) when receiving data (pack {}) from server", pkg_cnt));
        }

        recv_size = recv(server.socket, buffer, MAX_MSG_SIZE, 0);
        if (recv_size == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to receive data (pack {}) from server. Error code: {}", pkg_cnt, WSAGetLastError()));
        } else if (recv_size == 0) {
            break;
        }
        con<6>("{}:{} ------------- {}:{} <===[{}]==== {}:{}", client.ip, client.port, proxy_.ip, proxy_.port, recv_size, server.ip, server.port);

        ::std::cout << "DEBUG: about to send data to client: pack " << pkg_cnt << ::std::endl;
        if (send(client.socket, buffer, recv_size, 0) == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to send data (pack {}, {} bytes) to client. Error code: {}", pkg_cnt, recv_size, WSAGetLastError()));
        }
        total_size += recv_size;

        con<6>("{}:{} <===[{}]==== {}:{} ------------- {}:{} (total: {})", client.ip, client.port, recv_size, proxy_.ip, proxy_.port, server.ip, server.port, total_size);

        if (need_cache) {
            if (is_first) {
                // ::std::cout << "DEBUG: about to create cache for: " << c_req.url << ::std::endl;
                cache_manager_.create_cache(c_req.url);
            }
            // ::std::cout << "DEBUG: about to append cache for: " << c_req.url << ::std::endl;
            cache_manager_.append_cache(c_req.url, buffer, recv_size);
        }

        if (is_first) {
            // get status
            const char *start = strchr(buffer, ' ') + 1;
            const char *end = ::std::min(strchr(start, ' '), strchr(start, '\r'));
            status = ::std::string(start, end - start);
            is_first = false;
        }
    }

    if (need_cache) {
        if (total_size == 0) {
            cache_manager_.remove_cache(c_req.url);
        } else {
            // ::std::cout << "DEBUG: about to update cache time for: " << c_req.url << ::std::endl;
            if (cache_manager_.update_cache_time(c_req.url))
                log("Proxy<{}>: cache created(updated) for: {} ({})", p_no_, c_req.url, HttpCacheManager::get_key(c_req.url));
            else {
                log("Proxy<{}>: failed to create cache for: {} ({})", p_no_, c_req.url, HttpCacheManager::get_key(c_req.url));
                con<6>("Neither \"Last-Modified\" nor \"ETag\" found in response header");
            }
        }
    }
    return total_size;
}
