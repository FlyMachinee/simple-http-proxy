#include "../include/Host.h"

my::Host::Host(SOCKET socket, ::std::string ip, unsigned short port)
    : socket(socket), ip(ip), port(port)
{
}

my::Host::Host(SOCKET socket)
    : socket(socket)
{
    update();
}

my::Host::Host()
{
}

void my::Host::update()
{
    if (socket == INVALID_SOCKET) {
        ip = "";
        port = 0;
        return;
    }
    SOCKADDR_IN addr;
    int addr_len = sizeof(addr);
    getpeername(socket, reinterpret_cast<SOCKADDR *>(&addr), &addr_len);
    ip = inet_ntoa(addr.sin_addr);
    port = ntohs(addr.sin_port);
}