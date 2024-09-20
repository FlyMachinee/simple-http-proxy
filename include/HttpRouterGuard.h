#ifndef _HTTP_ROUTER_GUARD_H_INCLUDED_
#define _HTTP_ROUTER_GUARD_H_INCLUDED_

#include <map>
#include <set>
#include <string>

namespace my
{
    class HttpRouterGuard
    {
    public:
        enum class Response {
            OK,
            BLOCKED,
            REDIRECTED,
        };

        HttpRouterGuard() = default;
        ~HttpRouterGuard() = default;

        void add_client(::std::string_view ip);
        void remove_client(::std::string_view ip);
        Response check_client(::std::string_view ip) const;

        void add_server(::std::string_view url);
        void remove_server(::std::string_view url);
        void add_redirect(::std::string_view url, ::std::string_view redirect_url);
        void remove_redirect(::std::string_view url);

        Response check_server(::std::string_view url) const;
        ::std::string get_redirect_url(::std::string_view url) const;

    private:
        ::std::set<::std::string> client_blocked_;
        ::std::set<::std::string> server_blocked_;
        ::std::map<::std::string, ::std::string> server_redirect_map_;
    };
} // namespace my

#endif // _HTTP_ROUTER_GUARD_H_INCLUDED_