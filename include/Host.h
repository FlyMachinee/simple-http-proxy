#ifndef _HOST_H_INCLUDED_
#define _HOST_H_INCLUDED_

#include <string>
#include <winsock2.h>

namespace my
{
    // Host 结构体表示一个网络主机
    struct Host {
        SOCKET socket = INVALID_SOCKET; // 套接字，默认值为 INVALID_SOCKET
        ::std::string ip;               // 主机的 IP 地址
        unsigned short port = 0;        // 主机的端口号，默认值为 0

        // 构造函数，使用套接字、IP 地址和端口号初始化
        explicit Host(SOCKET socket, ::std::string ip, unsigned short port);
        // 构造函数，仅使用套接字初始化
        explicit Host(SOCKET socket);
        // 默认构造函数
        Host();

        // 更新主机信息的方法
        void update();
    };
} // namespace my

#endif // _HOST_H_INCLUDED_