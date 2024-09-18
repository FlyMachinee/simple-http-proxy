#ifndef _HOST_H_INCLUDED_
#define _HOST_H_INCLUDED_

#include <string>
#include <winsock2.h>

namespace my
{
    struct Host {
        SOCKET socket = INVALID_SOCKET;
        ::std::string ip;
        unsigned short port = 0;

        explicit Host(SOCKET socket, ::std::string ip, unsigned short port);
        explicit Host(SOCKET socket);
        Host();

        void update();
    };
} // namespace my

#endif // _HOST_H_INCLUDED_