#include "../include/Host.h"

// 构造函数，使用套接字、IP 地址和端口号初始化
my::Host::Host(SOCKET socket, ::std::string ip, unsigned short port)
    : socket(socket), ip(ip), port(port)
{
}

// 构造函数，仅使用套接字初始化，并调用 update 方法获取 IP 和端口号
my::Host::Host(SOCKET socket)
    : socket(socket)
{
    update();
}

// 默认构造函数
my::Host::Host()
{
}

// 更新主机信息的方法
void my::Host::update()
{
    if (socket == INVALID_SOCKET) {
        ip = "";
        port = 0;
        return;
    }
    SOCKADDR_IN addr;
    int addr_len = sizeof(addr);
    // 获取远程主机的地址信息
    getpeername(socket, reinterpret_cast<SOCKADDR *>(&addr), &addr_len);
    // 将地址信息转换为 IP 字符串和端口号
    ip = inet_ntoa(addr.sin_addr);
    port = ntohs(addr.sin_port);
}