#include <thread>

#include "../include/HttpProxyServer.h"
#include "../include/HttpRequest.h"
#include "../include/HttpResponseHead.h"
#include "../include/format_log.hpp"
#include "../include/util.h"
#include "../include/wsa_wapper.h"

// 初始化 HttpProxyServer 类的静态成员变量
int ::my::HttpProxyServer::instance_count_ = 0;
int ::my::HttpProxyServer::p_id_ = 0;

// 全局变量，表示是否检测到键盘中断
::std::atomic_bool keybord_interrupt = false;

// 键盘中断处理函数
// ctrl_type: 控制类型
// 返回值: 如果是 CTRL_C_EVENT 则返回 TRUE，否则返回 FALSE
BOOL WINAPI keybord_interrupt_handler(DWORD ctrl_type)
{
    if (ctrl_type == CTRL_C_EVENT) {
        keybord_interrupt.store(true); // 设置键盘中断标志
        return TRUE;
    }
    return FALSE;
}

// 构造函数
::my::HttpProxyServer::HttpProxyServer(const char *p_ip, unsigned short p_port, bool use_cache)
    : proxy_(INVALID_SOCKET, p_ip, p_port),
      use_cache_(use_cache), cache_manager_(".\\cache"), task_count_(0)
{
    log("Initializing proxy<{}> ...", p_id_);
    try {
        // 如果这是第一个实例且 Winsock 未初始化，则初始化 Winsock
        if (instance_count_ == 0 && !wsa_initialized) {
            log("Detected Winsock uninitialized");
            if (!init_wsa()) {
                throw std::runtime_error("Failed to initialize Winsock");
            }
        }

        // 创建套接字
        proxy_.socket = socket(AF_INET, SOCK_STREAM, 0);
        if (proxy_.socket == INVALID_SOCKET) {
            throw std::runtime_error(::std::format("Failed to create socket. Error code: {}", WSAGetLastError()));
        }
        log("Proxy<{}>: Socket created", p_id_);

        // 设置代理服务器地址
        SOCKADDR_IN proxy_addr;
        proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_port = htons(p_port);
        proxy_addr.sin_addr.s_addr = inet_addr(p_ip);

        // 绑定套接字到指定地址和端口
        if (bind(proxy_.socket, reinterpret_cast<SOCKADDR *>(&proxy_addr), sizeof(proxy_addr)) == SOCKET_ERROR) {
            throw std::runtime_error(::std::format("Failed to bind socket. Error code: {}", WSAGetLastError()));
        }
        con<6>("Socket bound to {}:{}", p_ip, p_port);

        // 监听套接字
        if (listen(proxy_.socket, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error(::std::format("Failed to listen on socket. Error code: {}", WSAGetLastError()));
        }
    } catch (const std::runtime_error &e) {
        // 如果发生异常，关闭套接字并记录错误信息
        if (proxy_.socket != INVALID_SOCKET) {
            closesocket(proxy_.socket);
        }
        err("In Proxy<{}> constructor:", p_id_);
        con<8>("{}", e.what());
        throw e;
    }
    con<6>("Proxy server will listen on {}:{}", p_ip, p_port);
    log("Proxy<{}> initialized\n", p_id_);
    instance_count_++; // 增加实例计数
    p_no_ = p_id_++;   // 设置代理服务器编号
}

// 析构函数
::my::HttpProxyServer::~HttpProxyServer()
{
    // 如果代理服务器的套接字有效，则关闭套接字
    if (proxy_.socket != INVALID_SOCKET) {
        closesocket(proxy_.socket);
        log("Proxy<{}>: Socket closed", p_no_);
    }
    log("Proxy<{}>: destroyed\n", p_no_);

    // 如果这是最后一个实例且 Winsock 已初始化，则清理 Winsock
    if (--instance_count_ == 0 && wsa_initialized) {
        if (!cleanup_wsa()) {
            err("In proxy<{}> destructor:", p_no_);
            con<8>("Failed to clean up Winsock");
        }
    }
}

// 运行代理服务器（单线程模式）
// 返回值: 如果成功运行则返回 true，否则返回 false
bool ::my::HttpProxyServer::run()
{
    return inner_run(false);
}

// 运行代理服务器（多线程模式）
// 返回值: 如果成功运行则返回 true，否则返回 false
bool my::HttpProxyServer::run_multithread()
{
    return inner_run(true);
}

// 检查代理服务器是否正在运行
// 返回值: 如果正在运行则返回 true，否则返回 false
bool my::HttpProxyServer::is_running() const
{
    return is_running_;
}

// 获取路由守护对象
// 返回值: 路由守护对象的引用
::my::HttpRouterGuard &my::HttpProxyServer::router_guard()
{
    return router_guard_;
}

bool my::HttpProxyServer::inner_run(bool is_multithread)
{
    if (is_running_) {
        err("Proxy<{}>: is already running", p_no_);
        return false;
    }
    is_running_ = true;

    // 添加ctrl+c中断处理函数
    SetConsoleCtrlHandler(keybord_interrupt_handler, TRUE);

    log("Proxy<{}>: is running...\n", p_no_);

    // 超时处理
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(proxy_.socket, &readfds);
    TIMEVAL timeout = {0, 0};

    Host client;
    int client_cnt = 0;

    while (!keybord_interrupt) {

        // 如果没有客户端连接，则等待客户端连接
        while (!keybord_interrupt) {
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
        if (keybord_interrupt) {
            break;
        }

        // 接受客户端连接
        client.socket = accept(proxy_.socket, nullptr, nullptr);
        if (client.socket == INVALID_SOCKET) {
            err("In Proxy<{}>:", p_no_);
            con<8>("Failed to accept client. Error code: {}", WSAGetLastError());
            con<8>("Continue listening...");
            continue;
        }
        client.update();

        // 检查客户端 IP 是否被阻止
        if (router_guard_.check_client(client.ip) == HttpRouterGuard::Response::BLOCKED) {
            log("Proxy<{}>: client<{}> ip: \"{}\" is blocked, rejected", p_no_, client_cnt, client.ip);
            send(client.socket, "HTTP/1.1 403 Forbidden\r\n\r\n", 26, 0);
            closesocket(client.socket);
            continue;
        }

        ++client_cnt;
        task_count_++;

        // 处理客户端请求
        if (is_multithread) {
            // 多线程模式
            thread_pool_.add_task(handle_client, this, client_cnt, client);
        } else {
            // 单线程模式
            con<0>("====================[task {}]====================", client_cnt);
            handle_client(client_cnt, client);
            con<0>("====================[task {}]====================\n", client_cnt);
        }
    }

    // 移除ctrl+c中断处理函数
    SetConsoleCtrlHandler(keybord_interrupt_handler, FALSE);

    log("Proxy<{}>: Keyboard interrupt detected, stopping...", p_no_);
    if (task_count_ > 0) {
        log("Waiting for {} tasks to finish...", task_count_.load());
        thread_pool_.wait_all();
    }

    log("Proxy<{}>: stopped\n", p_no_);
    is_running_ = false;
    return true;
}

void ::my::HttpProxyServer::handle_client(int c_no, Host client)
{
    // HOSTENT *s_hostent = nullptr;
    Host server;
    ::std::string s_hostname;
    int recv_cnt, recv_size, send_size;

    // 请求处理逻辑
    try {
        log("Proxy<{}>: connected with client<{}>: {}:{}", p_no_, c_no, client.ip, client.port);
        con<6>("{}:{} ------------- {}:{} - - - - ?:?", client.ip, client.port, proxy_.ip, proxy_.port);

        char buffer[MAX_BUFFER_SIZE];

        // 接收客户端请求
        recv_size = recv_with_timeout(client.socket, buffer, MAX_BUFFER_SIZE, {1, 0});
        if (recv_size == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to receive data from client<{}>. Error code: {}", c_no, WSAGetLastError()));
        } else if (recv_size == 0) {
            throw ::std::runtime_error(::std::format("Client<{}> disconnected", c_no));
        }

        // 通过客户端请求解析出服务器主机名和端口号
        HttpRequest c_req(buffer, recv_size);
        ::std::tie(s_hostname, server.port) = c_req.get_host_port();
        server.ip = get_ip_str(s_hostname.c_str());

        log("Proxy<{}>: received {} bytes data from client<{}> successfully:", p_no_, recv_size, c_no);
        // out_http_data(10, buffer, recv_size);
        con<6>("{}:{} ====[{}]===> {}:{} - - - - - - - {}:{} ({})", client.ip, client.port, c_req.method, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);
        con<6>("URL: {}", c_req.url);

        // 检查服务器 IP 是否被阻止
        HttpRouterGuard::Response response = router_guard_.check_server(c_req.url);

        // 根据检查结果进行处理
        if (response == HttpRouterGuard::Response::BLOCKED) {
            // 如果服务器 IP 被阻止，则返回 403 Forbidden
            send(client.socket, "HTTP/1.1 403 Forbidden\r\n\r\n", 26, 0);

            log("Proxy<{}>: requesting url: \"{}\" is blocked, rejected", p_no_, c_req.url);
            con<6>("{}:{} <====[ 403 ]==== {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);

        } else if (response == HttpRouterGuard::Response::REDIRECTED) {
            // 如果服务器 IP 被重定向，则返回 302 Found
            // 并且返回重定向的 URL
            ::std::string redirect_url = router_guard_.get_redirect_url(c_req.url);
            ::std::string response = "HTTP/1.1 302 Found\r\nLocation: " + redirect_url + "\r\n\r\n";

            send(client.socket, response.c_str(), response.length(), 0);

            log("Proxy<{}>: requesting url: \"{}\" is redirected to \"{}\"", p_no_, c_req.url, redirect_url);
            con<6>("{}:{} <====[ 302 ]==== {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);

        } else if (c_req.method == "GET" || c_req.method == "POST") {
            // 如果是 GET 或 POST 请求，则连接到服务器
            try {
                server.socket = connect_to_server(server.ip.c_str(), server.port);
            } catch (const ::std::runtime_error &e) {
                throw ::std::runtime_error(::std::format("During connecting to server {}:{}", server.ip, server.port) + "\n        " + e.what());
            }

            log("Proxy<{}>: enstabished connection with server {}", p_no_, s_hostname);
            con<6>("{}:{} ------------- {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);

            // ::std::cout << "DEBUG: about to check cache" << ::std::endl;
            // 检查cache并接收第一个数据包
            CheckCacheResult chk_res = check_cache_and_recv(c_req, server, buffer, MAX_BUFFER_SIZE, recv_size);

            if (chk_res == CheckCacheResult::FOUND) {
                // 如果缓存命中，则从缓存中响应请求
                log("Proxy<{}>: cache hit for: {} ({})", p_no_, c_req.url, HttpCacheManager::get_key(c_req.url));

                int total_size = answer_from_cache(c_req.url, client);
                log("Proxy<{}>: transmitted {} bytes data from cache to client<{}> successfully", p_no_, total_size, c_no);
                con<6>("{}:{} <==[cached]== {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);

            } else {
                // 否则继续从服务器接收数据
                // ::std::cout << "DEBUG: about to answer from server" << ::std::endl;
                ::std::string status(strchr(buffer, ' ') + 1, 3);

                int total_size = answer_from_server(chk_res, c_req.url, client, server, buffer, MAX_BUFFER_SIZE, recv_size);

                log("Proxy<{}>: transmitted {} bytes data from server {} to client<{}> successfully", p_no_, total_size, s_hostname, c_no);
                con<6>("{}:{} <================[ {} ]================= {}:{} ({})", client.ip, client.port, status, server.ip, server.port, s_hostname);
            }
        } else {
            // 如果是其他请求方法，则返回 405 Method Not Allowed
            send(client.socket, "HTTP/1.1 405 Method Not Allowed\r\n\r\n", 34, 0);

            log("Proxy<{}>: received an unsupported method {} from client<{}>, rejected", p_no_, c_req.method, c_no);
            con<6>("{}:{} <====[ 405 ]==== {}:{} ------------- {}:{} ({})", client.ip, client.port, proxy_.ip, proxy_.port, server.ip, server.port, s_hostname);
        }
    } catch (const ::std::exception &e) {
        err("In Proxy<{}>:", p_no_);
        con<8>("{}", e.what());
    }

    if (server.socket != INVALID_SOCKET) {
        closesocket(server.socket);
        log("Proxy<{}>: disconnected with server {}", p_no_, s_hostname);
    }
    closesocket(client.socket);
    log("Proxy<{}>: disconnected with client<{}>", p_no_, c_no);

    --task_count_;
}

// 检查缓存并接收第一个数据包
my::CheckCacheResult
my::HttpProxyServer::check_cache_and_recv(
    HttpRequest client_request, const Host &server, char *buffer, int buf_size, int &recv_size)
{
    CheckCacheResult chk_res = CheckCacheResult::NONE;
    if (!use_cache_ || client_request.method != "GET") {
        chk_res = CheckCacheResult::NOT_SUPPORTED;
    } else if (!cache_manager_.has_cache(client_request.url)) {
        chk_res = CheckCacheResult::NO_CACHE;
    }

    // 移除客户端请求中的代理相关头部，添加 Connection: close 头部
    client_request.headers.erase("Proxy-Connection");
    client_request.headers["Connection"] = "close";

    // 如果缓存存在，则添加 If-Modified-Since 和 If-None-Match 头部
    if (chk_res == CheckCacheResult::NONE) {
        if (cache_manager_.get_modified_time(client_request.url) != "") {
            client_request.headers["If-Modified-Since"] = cache_manager_.get_modified_time(client_request.url);
        }
        if (cache_manager_.get_etag(client_request.url) != "") {
            client_request.headers["If-None-Match"] = cache_manager_.get_etag(client_request.url);
        }
    }
    recv_size = send_and_recv(client_request, server, buffer, buf_size);

    if (chk_res == CheckCacheResult::NONE) {
        const char *start = strchr(buffer, ' ') + 1;
        ::std::string status = ::std::string(start, 3);
        if (status == "304") {
            chk_res = CheckCacheResult::FOUND;
        } else if (status == "200") {
            chk_res = CheckCacheResult::EXPIRED;
        } else {
            throw ::std::runtime_error(::std::format("Unexpected status code: {} received from server when checking cache", status));
        }
    }
    return chk_res;
}

// 从缓存中响应请求
int my::HttpProxyServer::answer_from_cache(::std::string_view url, const Host &client)
{
    char buffer[MAX_BUFFER_SIZE];
    int total_size = 0;
    int read_size;
    int pkg_cnt = 0;
    // 从缓存中读取数据并发送给客户端
    while ((read_size = cache_manager_.read_cache(url, buffer, MAX_BUFFER_SIZE, total_size)) > 0) {
        if (send(client.socket, buffer, read_size, 0) == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to send data (pack {}, {} bytes) to client. Error code: {}", pkg_cnt, read_size, WSAGetLastError()));
        }
        total_size += read_size;
        ++pkg_cnt;
    }
    return total_size;
}

// 发送请求并接收响应
int my::HttpProxyServer::send_and_recv(const HttpRequest &req, const Host &server, char *buffer, int buf_size)
{
    ::std::string check_request = req.to_string();
    // ::std::cout << "DEBUG: check request: " << check_request << ::std::endl;
    if (send(server.socket, check_request.c_str(), check_request.length(), 0) == SOCKET_ERROR) {
        throw ::std::runtime_error(::std::format("Failed to send check request to server. Error code: {}", WSAGetLastError()));
    }

    int recv_size = recv_with_timeout(server.socket, buffer, buf_size, {1, 0});
    if (recv_size == SOCKET_ERROR) {
        throw ::std::runtime_error(::std::format("Failed to receive data from server. Error code: {}", WSAGetLastError()));
    } else if (recv_size == 0) {
        throw ::std::runtime_error("Server disconnected when receiving data");
    }

    return recv_size;
}

// 从服务器响应请求
int my::HttpProxyServer::answer_from_server(CheckCacheResult chk_res, ::std::string_view url, const Host &client, const Host &server, char *buffer, int buf_size, int recv_size)
{
    int total_size = 0;
    bool need_cache = chk_res == CheckCacheResult::EXPIRED || chk_res == CheckCacheResult::NO_CACHE;
    int pkg_cnt = 0;

    if (need_cache) {
        cache_manager_.create_cache(url);
    }

    // 发送第一个数据包给客户端
    // 然后继续接收数据并发送给客户端

    while (recv_size > 0) {
        // ::std::cout << "DEBUG: about to receive data from server: pack " << pkg_cnt << ::std::endl;

        con<6>("{}:{} ------------- {}:{} <===[{}]==== {}:{}", client.ip, client.port, proxy_.ip, proxy_.port, recv_size, server.ip, server.port);

        // ::std::cout << "DEBUG: about to send data to client: pack " << pkg_cnt << ::std::endl;
        if (send(client.socket, buffer, recv_size, 0) == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to send data (pack {}, {} bytes) to client. Error code: {}", pkg_cnt, recv_size, WSAGetLastError()));
        }
        total_size += recv_size;

        con<6>("{}:{} <===[{}]==== {}:{} ------------- {}:{} (total: {})", client.ip, client.port, recv_size, proxy_.ip, proxy_.port, server.ip, server.port, total_size);

        if (need_cache) {
            // ::std::cout << "DEBUG: about to append cache for: " << c_req.url << ::std::endl;
            cache_manager_.append_cache(url, buffer, recv_size);
        }
        ++pkg_cnt;

        recv_size = recv_with_timeout(server.socket, buffer, buf_size, {1, 0});
        if (recv_size == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to receive data (pack {}) from server. Error code: {}", pkg_cnt, WSAGetLastError()));
        }
    }

    // 判断是否需要更新缓存时间
    if (need_cache) {
        if (total_size == 0) {
            cache_manager_.remove_cache(url);
        } else {
            // ::std::cout << "DEBUG: about to update cache time for: " << c_req.url << ::std::endl;
            if (cache_manager_.update_cache_time(url))
                log("Proxy<{}>: cache created(updated) for: {} ({})", p_no_, url, HttpCacheManager::get_key(url));
            else {
                log("Proxy<{}>: refused to create cache for: {} ({})", p_no_, url, HttpCacheManager::get_key(url));
                con<6>("Neither \"Last-Modified\" nor \"ETag\" found in response header");
            }
        }
    }
    return total_size;
}
