#ifndef _WSA_WRAPPER_H_INCLUDED_
#define _WSA_WRAPPER_H_INCLUDED_

#include <winsock2.h>

namespace my
{
    extern bool wsa_initialized; // 表示 WSA 是否已初始化的全局变量

    // 初始化 WSA（Windows Sockets API）
    bool init_wsa();

    // 清理 WSA
    bool cleanup_wsa();

    // 连接到服务器
    // s_ip: 服务器 IP 地址
    // s_port: 服务器端口号
    // 返回值: 连接的套接字
    SOCKET connect_to_server(const char *s_ip, unsigned short s_port);

    // 获取主机的 IP 字符串
    // host: 主机名
    // 返回值: 主机的 IP 地址字符串
    const char *get_ip_str(const char *host);

    // 带超时的接收数据
    // s: 套接字
    // buffer: 接收缓冲区
    // buf_size: 缓冲区大小
    // timeout: 超时时间
    // 返回值: 接收到的字节数
    int recv_with_timeout(SOCKET s, char *buffer, int buf_size, TIMEVAL timeout);
} // namespace my

#endif // _WSA_WRAPPER_H_INCLUDED_