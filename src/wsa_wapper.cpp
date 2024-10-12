#include "../include/wsa_wapper.h"
#include "../include/format_log.hpp"
#include <format>

// 全局变量，表示 WSA 是否已初始化
bool ::my::wsa_initialized = false;

// 初始化 WSA（Windows Sockets API）
bool ::my::init_wsa()
{
    if (wsa_initialized) {
        log("Winsock already initialized.");
        return true;
    }

    WSADATA wsaData;
    log("Initializing Winsock...");
    con<6>("Expected Winsock version: {}.{}", HIBYTE(WINSOCK_VERSION), LOBYTE(WINSOCK_VERSION));

    // 初始化 Winsock
    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
        err("WSAStartup failed. Error code: {}", WSAGetLastError());
        con<8>("Winsock initialization failed.");
        return false;
    }

    log("WSAStartup succeeded.");

    // 检查 Winsock 版本是否匹配
    if (LOBYTE(wsaData.wVersion) != LOBYTE(WINSOCK_VERSION) || HIBYTE(wsaData.wVersion) != HIBYTE(WINSOCK_VERSION)) {
        err("Winsock version not supported.");
        con<8>("Get: {}.{}", HIBYTE(wsaData.wVersion), LOBYTE(wsaData.wVersion));
        con<8>("Winsock initialization failed.");
        WSACleanup();
        return false;
    }

    log("Winsock initialized.");
    wsa_initialized = true;
    return true;
}

// 清理 WSA
bool ::my::cleanup_wsa()
{
    if (!wsa_initialized) {
        log("Winsock not initialized.");
        return false;
    }

    log("Cleaning up Winsock...");
    if (WSACleanup()) {
        err("WSACleanup failed. Error code: {}", WSAGetLastError());
        con<8>("Winsock cleanup failed.");
        return false;
    }

    log("WSACleanup succeeded.");
    log("Winsock cleaned up.");
    wsa_initialized = false;
    return true;
}

// 连接到服务器
SOCKET my::connect_to_server(const char *s_ip, unsigned short s_port)
{
    // 创建套接字
    SOCKET s_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sock == INVALID_SOCKET) {
        throw std::runtime_error(::std::format("Failed to create socket. Error code: {}", WSAGetLastError()));
    }

    // 设置服务器地址
    SOCKADDR_IN s_sockaddr;
    s_sockaddr.sin_family = AF_INET;
    s_sockaddr.sin_port = htons(s_port);
    s_sockaddr.sin_addr.s_addr = inet_addr(s_ip);

    // 连接到服务器
    if (connect(s_sock, reinterpret_cast<SOCKADDR *>(&s_sockaddr), sizeof(s_sockaddr)) == SOCKET_ERROR) {
        closesocket(s_sock);
        throw std::runtime_error(::std::format("Failed to connect to server. Error code: {}", WSAGetLastError()));
    }

    return s_sock;
}

// 获取主机的 IP 字符串
const char *my::get_ip_str(const char *host)
{
    // 解析主机名
    hostent *host_info = gethostbyname(host);
    if (host_info == nullptr) {
        throw std::runtime_error(::std::format("Failed to resolve host: {}. Error code: {}", host, WSAGetLastError()));
    }

    // 返回 IP 地址字符串
    return inet_ntoa(*reinterpret_cast<IN_ADDR *>(host_info->h_addr));
}

// 带超时的接收数据
int my::recv_with_timeout(SOCKET s, char *buffer, int buf_size, TIMEVAL timeout)
{
    fd_set readfds;
    FD_ZERO(&readfds);   // 清空文件描述符集合
    FD_SET(s, &readfds); // 将套接字加入集合
    // 使用 select 函数等待数据可读或超时
    if (select(0, &readfds, nullptr, nullptr, &timeout) == 0) {
        throw ::std::runtime_error("Timeout(1s) when receiving data from server");
    }
    // 接收数据
    return recv(s, buffer, buf_size, 0);
}