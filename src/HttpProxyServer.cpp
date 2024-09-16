#include <chrono>
#include <thread>

#include "../include/HttpProxyServer.h"
#include "../include/HttpRequest.h"
#include "../include/format_log.hpp"
#include "../include/util.h"
#include "../include/wsa_wapper.h"

int ::my::HttpProxyServer::instance_count = 0;

::my::HttpProxyServer::HttpProxyServer(const char *p_ip, unsigned short p_port, bool use_cache)
    : p_ip(p_ip), p_port(p_port), use_cache(use_cache)
{
    log("Initializing proxy server {}...", instance_count);

    try {
        if (instance_count == 0 && !wsa_initialized) {
            log("Detected Winsock uninitialized.");
            if (!init_wsa()) {
                throw std::runtime_error("Failed to initialize Winsock.");
            }
        }

        proxy_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (proxy_socket == INVALID_SOCKET) {
            throw std::runtime_error(::std::format("Failed to create socket. Error code: {}", WSAGetLastError()));
        }
        log("Socket created.");

        SOCKADDR_IN proxy_addr;
        proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_port = htons(p_port);
        proxy_addr.sin_addr.s_addr = inet_addr(p_ip);

        if (bind(proxy_socket, reinterpret_cast<SOCKADDR *>(&proxy_addr), sizeof(proxy_addr)) == SOCKET_ERROR) {
            closesocket(proxy_socket);
            throw std::runtime_error(::std::format("Failed to bind socket. Error code: {}", WSAGetLastError()));
        }
        log("Socket bound to {}:{}", p_ip, p_port);

        if (listen(proxy_socket, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(proxy_socket);
            throw std::runtime_error(::std::format("Failed to listen on socket. Error code: {}", WSAGetLastError()));
        }
    } catch (const std::runtime_error &e) {
        err("In proxy_server {}'s constructor:", instance_count);
        con<8>("{}", e.what());
        throw e;
    }

    log("Proxy server wil listen on {}:{}", p_ip, p_port);
    log("Proxy server {} initialized.\n", instance_count);

    p_no = instance_count++;
}

::my::HttpProxyServer::~HttpProxyServer()
{
    if (proxy_socket != INVALID_SOCKET) {
        closesocket(proxy_socket);
        log("Socket closed.");
    }

    log("Proxy server {} destroyed.\n", p_no);

    if (--instance_count == 0 && wsa_initialized) {
        if (!cleanup_wsa()) {
            err("In proxy_server {}'s destructor:", p_no);
            con<8>("Failed to clean up Winsock.");
            err("Program exit abnormally.");
            exit(1);
        }
    }
}

bool ::my::HttpProxyServer::run()
{
    log("Proxy server {} is running, listening on {}:{} ...\n", p_no, p_ip, p_port);

    SOCKET client_socket = INVALID_SOCKET;
    SOCKADDR_IN client_addr;

    int client_cnt = 0;
    while (true) {
        client_socket = accept(proxy_socket, reinterpret_cast<SOCKADDR *>(&client_addr), nullptr);
        if (client_socket == INVALID_SOCKET) {
            err("In proxy_server {}'s run:", p_no);
            con<8>("Failed to accept client. Error code: {}", WSAGetLastError());
            con<8>("Continue listening...");
            continue;
        }

        const char *client_ip = inet_ntoa(client_addr.sin_addr);
        unsigned short client_port = ntohs(client_addr.sin_port);
        ++client_cnt;

        // con<0>("==========================| thread {} |=============================\n", client_cnt);
        ::std::thread(&::my::HttpProxyServer::handle_client, this, client_cnt, client_socket, client_ip, client_port).detach();
        // con<0>("=======================| end of thread {} |=========================\n\n", client_cnt);

        // ::std::this_thread::sleep_for(::std::chrono::milliseconds(100));
    }

    log("Proxy server {} stopped.\n", p_no);
    return true;
}

void ::my::HttpProxyServer::handle_client(int c_no, SOCKET c_sock, const char *c_ip, unsigned short c_port)
{
    HOSTENT *s_hostent = nullptr;
    const char *s_ip = nullptr;
    unsigned short s_port;
    SOCKET s_sock = INVALID_SOCKET;
    int recv_cnt, recv_size, send_size;

    try {

        log("Proxy_server {} connected with client {}: {}:{}\n", p_no, c_no, c_ip, c_port);

        char buffer[MAX_MSG_SIZE];

        recv_size = recv(c_sock, buffer, MAX_MSG_SIZE, 0);
        if (recv_size == SOCKET_ERROR) {
            throw ::std::runtime_error(::std::format("Failed to receive data from client {}. Error code: {}", c_no, WSAGetLastError()));
        }

        log("Proxy_server {} received {} bytes data from client {} successfully:", p_no, recv_size, c_no);
        out_http_data(10, buffer, recv_size);
        out(0, "");

        // ::std::cout << "# DEBUG \n"
        //             << ::std::string_view(buffer, recv_size) << '\n';

        HttpRequest c_req(buffer, recv_size);

        ::std::string s_hostname;
        ::std::tie(s_hostname, s_port) = c_req.get_host_port();
        s_ip = get_ip_str(s_hostname.c_str());

        if (c_req.method == "GET" || c_req.method == "POST") {
            try {
                s_sock = connect_to_server(s_ip, s_port);
            } catch (const ::std::runtime_error &e) {
                throw ::std::runtime_error(::std::format("During connecting to server {}:{}", s_ip, s_port) + "\n        " + e.what());
            }

            log("Proxy_server {} enstabished connection with server", p_no, s_ip, s_port);
            con<6>("Hosts: {}:{} <=> {}:{} <=> {}:{} ({})", c_ip, c_port, p_ip, p_port, s_ip, s_port, s_hostname);

            send_size = send(s_sock, buffer, recv_size, 0);
            if (send_size == SOCKET_ERROR) {
                throw ::std::runtime_error(::std::format("Failed to send data to server {}:{}.", s_ip, s_port));
            }

            log("Proxy_server {} sent {} bytes data to server {}:{} successfully.", p_no, send_size, s_ip, s_port);

            recv_size = recv(s_sock, buffer, MAX_MSG_SIZE, 0);
            if (recv_size == SOCKET_ERROR) {
                throw ::std::runtime_error(::std::format("Failed to receive data from server {}:{}.", s_ip, s_port));
            }

            log("Proxy_server {} received {} bytes data from server {}:{} successfully:", p_no, recv_size, s_ip, s_port);
            out_http_data(10, buffer, recv_size);
            out(0, "");

            send_size = send(c_sock, buffer, recv_size, 0);
            if (send_size == SOCKET_ERROR) {
                throw ::std::runtime_error(::std::format("Failed to send data to client {}.", c_no));
            }

            log("Proxy_server {} sent {} bytes data to client {} successfully.", p_no, send_size, c_no);
        }
    } catch (const ::std::exception &e) {
        err("In proxy_server {}:", p_no);
        con<8>("{}\n", e.what());
    }

    if (s_sock != INVALID_SOCKET) {
        closesocket(s_sock);
        log("Proxy_server {} disconnected from server {}:{}.", p_no, s_ip, s_port);
    }
    closesocket(c_sock);
    log("Proxy_server {} disconnected from client {}: {}:{}.\n", p_no, c_no, c_ip, c_port);
}
