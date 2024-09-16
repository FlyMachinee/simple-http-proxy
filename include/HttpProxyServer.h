#ifndef _HTTP_PROXY_SERVER_H_INCLUDED_
#define _HTTP_PROXY_SERVER_H_INCLUDED_

#include <winsock2.h>

namespace my
{
    class HttpProxyServer
    {
    private:
        static int instance_count;
        static constexpr int MAX_MSG_SIZE = 1 << 18;

        int p_no;
        const char *p_ip;
        unsigned short p_port;
        bool use_cache;

        SOCKET proxy_socket = INVALID_SOCKET;

    public:
        HttpProxyServer(const char *p_ip, unsigned short p_port, bool use_cache = false);
        ~HttpProxyServer();

        bool run();

    private:
        void handle_client(int c_no, SOCKET c_sock, const char *c_ip, unsigned short c_port);

    }; // class proxy_server

} // namespace my

#endif // _HTTP_PROXY_SERVER_H_INCLUDED_