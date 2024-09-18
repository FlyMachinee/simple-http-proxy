#ifndef _HTTP_Proxy_H_INCLUDED_
#define _HTTP_Proxy_H_INCLUDED_

#include "./Host.h"
#include "./HttpCacheManager.h"
#include "./HttpRequest.h"
#include <atomic>
#include <winsock2.h>

#include <windows.h>

namespace my
{
    class HttpProxyServer
    {
    private:
        static int instance_count_;
        static int p_id_;

        int p_no_;
        Host proxy_;

        ::std::atomic<int> task_count_;

        bool use_cache_;
        HttpCacheManager cache_manager_;

    public:
        static constexpr int MAX_MSG_SIZE = 65535;

        HttpProxyServer(const char *p_ip, unsigned short p_port, bool use_cache = false);
        ~HttpProxyServer();

        bool run();

    private:
        void handle_client(int c_no, Host client);

        bool check_cache(const HttpRequest &client_request, const Host &server);
        int answer_from_cache(::std::string_view url, const Host &client);
        int answer_from_server(HttpRequest client_request, const Host &client, const Host &server, ::std::string &status);

    }; // class Proxy

} // namespace my

#endif // _HTTP_Proxy_H_INCLUDED_