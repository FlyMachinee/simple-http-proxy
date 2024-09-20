#ifndef _HTTP_PROXY_SERVER_H_INCLUDED_
#define _HTTP_PROXY_SERVER_H_INCLUDED_

#include "./Host.h"
#include "./HttpCacheManager.h"
#include "./HttpRequest.h"
#include "./HttpRouterGuard.h"
#include "./SimpleThreadPool.hpp"
#include <atomic>
#include <winsock2.h>

#include <windows.h>

namespace my
{
    enum class CheckCacheResult {
        NONE,
        NOT_SUPPORTED,
        NO_CACHE,
        FOUND,
        EXPIRED,
    };

    class HttpProxyServer
    {
    public:
        static constexpr int MAX_BUFFER_SIZE = 65535;

        HttpProxyServer(const char *p_ip, unsigned short p_port, bool use_cache = false);
        ~HttpProxyServer();

        bool run();
        bool run_multithread();
        bool is_running() const;

        HttpRouterGuard &router_guard();

        HttpProxyServer(const HttpProxyServer &) = delete;
        HttpProxyServer &operator=(const HttpProxyServer &) = delete;
        HttpProxyServer(HttpProxyServer &&) = delete;
        HttpProxyServer &operator=(HttpProxyServer &&) = delete;

    private:
        bool inner_run(bool is_multithread);
        void handle_client(int c_no, Host client);

        CheckCacheResult check_cache_and_recv(HttpRequest client_request, const Host &server, char *buffer, int buf_size, int &recv_size);
        int answer_from_cache(::std::string_view url, const Host &client);
        int answer_from_server(CheckCacheResult chk_res, ::std::string_view url, const Host &client, const Host &server, char *buffer, int buf_size, int recv_size);

        int send_and_recv(const HttpRequest &request, const Host &host, char *recv_buffer, int buf_size);

        int p_no_;
        Host proxy_;
        bool use_cache_;
        HttpCacheManager cache_manager_;
        HttpRouterGuard router_guard_;
        ::std::atomic_int task_count_;
        ::std::atomic_bool is_running_;

        SimpleThreadPool thread_pool_;

        static int instance_count_;
        static int p_id_;
    }; // class HttpProxyServer

} // namespace my

#endif // _HTTP_PROXY_SERVER_H_INCLUDED_