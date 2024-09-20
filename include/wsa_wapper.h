#ifndef _WSA_WRAPPER_H_INCLUDED_
#define _WSA_WRAPPER_H_INCLUDED_

#include <winsock2.h>

namespace my
{
    extern bool wsa_initialized;
    bool init_wsa();
    bool cleanup_wsa();
    SOCKET connect_to_server(const char *s_ip, unsigned short s_port);
    const char *get_ip_str(const char *host);
    int recv_with_timeout(SOCKET s, char *buffer, int buf_size, TIMEVAL timeout);
} // namespace my

#endif // _WSA_WRAPPER_H_INCLUDED_