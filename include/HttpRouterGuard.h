#ifndef _HTTP_ROUTER_GUARD_H_INCLUDED_
#define _HTTP_ROUTER_GUARD_H_INCLUDED_

#include <map>
#include <set>
#include <string>

namespace my
{
    // HttpRouterGuard 类用于管理 HTTP 路由的访问控制
    class HttpRouterGuard
    {
    public:
        // Response 枚举表示检查结果
        enum class Response {
            OK,         // 访问正常
            BLOCKED,    // 访问被阻止
            REDIRECTED, // 访问被重定向
        };

        // 默认构造函数
        HttpRouterGuard() = default;
        // 默认析构函数
        ~HttpRouterGuard() = default;

        // 添加被阻止的客户端 IP
        void add_client(::std::string_view ip);
        // 移除被阻止的客户端 IP
        void remove_client(::std::string_view ip);
        // 检查客户端 IP 是否被阻止
        Response check_client(::std::string_view ip) const;

        // 添加被阻止的服务器 URL
        void add_server(::std::string_view url);
        // 移除被阻止的服务器 URL
        void remove_server(::std::string_view url);
        // 添加服务器 URL 重定向
        void add_redirect(::std::string_view url, ::std::string_view redirect_url);
        // 移除服务器 URL 重定向
        void remove_redirect(::std::string_view url);

        // 检查服务器 URL 是否被阻止或重定向
        Response check_server(::std::string_view url) const;
        // 获取服务器 URL 的重定向地址
        ::std::string get_redirect_url(::std::string_view url) const;

    private:
        ::std::set<::std::string> client_blocked_;                     // 被阻止的客户端 IP 集合
        ::std::set<::std::string> server_blocked_;                     // 被阻止的服务器 URL 集合
        ::std::map<::std::string, ::std::string> server_redirect_map_; // 服务器 URL 重定向映射
    };
} // namespace my

#endif // _HTTP_ROUTER_GUARD_H_INCLUDED_