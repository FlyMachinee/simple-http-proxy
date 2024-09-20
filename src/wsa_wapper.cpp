#include "../include/wsa_wapper.h"
#include "../include/format_log.hpp"
#include <format>

bool ::my::wsa_initialized = false;

/**
 * @brief Initialize Winsock.
 * @return true if Winsock is successfully initialized, false otherwise.
 */
bool ::my::init_wsa()
{
    if (wsa_initialized) {
        log("Winsock already initialized.");
        return true;
    }

    WSADATA wsaData;
    log("Initializing Winsock...");
    con<6>("Expected Winsock version: {}.{}", HIBYTE(WINSOCK_VERSION), LOBYTE(WINSOCK_VERSION));

    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
        err("WSAStartup failed. Error code: {}", WSAGetLastError());
        con<8>("Winsock initialization failed.");
        return false;
    }

    log("WSAStartup succeeded.");

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

/**
 * @brief Clean up Winsock.
 * @return true if Winsock is successfully cleaned up, false otherwise.
 */
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

SOCKET my::connect_to_server(const char *s_ip, unsigned short s_port)
{
    SOCKET s_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sock == INVALID_SOCKET) {
        throw std::runtime_error(::std::format("Failed to create socket. Error code: {}", WSAGetLastError()));
    }

    SOCKADDR_IN s_sockaddr;
    s_sockaddr.sin_family = AF_INET;
    s_sockaddr.sin_port = htons(s_port);
    s_sockaddr.sin_addr.s_addr = inet_addr(s_ip);

    if (connect(s_sock, reinterpret_cast<SOCKADDR *>(&s_sockaddr), sizeof(s_sockaddr)) == SOCKET_ERROR) {
        closesocket(s_sock);
        throw std::runtime_error(::std::format("Failed to connect to server. Error code: {}", WSAGetLastError()));
    }

    return s_sock;
}

const char *my::get_ip_str(const char *host)
{
    hostent *host_info = gethostbyname(host);
    if (host_info == nullptr) {
        throw std::runtime_error(::std::format("Failed to resolve host: {}. Error code: {}", host, WSAGetLastError()));
    }

    return inet_ntoa(*reinterpret_cast<IN_ADDR *>(host_info->h_addr));
}

int my::recv_with_timeout(SOCKET s, char *buffer, int buf_size, TIMEVAL timeout)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(s, &readfds);
    if (select(0, &readfds, nullptr, nullptr, &timeout) == 0) {
        throw ::std::runtime_error("Timeout(1s) when receiving data from server");
    }
    return recv(s, buffer, buf_size, 0);
}