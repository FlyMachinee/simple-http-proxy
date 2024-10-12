#include "../include/HttpRouterGuard.h"

// 添加被阻止的客户端 IP
void ::my::HttpRouterGuard::add_client(::std::string_view ip)
{
    client_blocked_.insert(::std::string(ip));
}

// 移除被阻止的客户端 IP
void ::my::HttpRouterGuard::remove_client(::std::string_view ip)
{
    client_blocked_.erase(::std::string(ip));
}

// 检查客户端 IP 是否被阻止
::my::HttpRouterGuard::Response my::HttpRouterGuard::check_client(::std::string_view ip) const
{
    if (client_blocked_.find(::std::string(ip)) != client_blocked_.end()) {
        return Response::BLOCKED; // 如果在被阻止的客户端列表中，则返回 BLOCKED
    }
    return Response::OK; // 否则返回 OK
}

// 添加被阻止的服务器 URL
void ::my::HttpRouterGuard::add_server(::std::string_view url)
{
    server_blocked_.insert(::std::string(url));
}

// 移除被阻止的服务器 URL
void ::my::HttpRouterGuard::remove_server(::std::string_view url)
{
    server_blocked_.erase(::std::string(url));
}

// 添加服务器 URL 重定向
void my::HttpRouterGuard::add_redirect(::std::string_view url, ::std::string_view redirect_url)
{
    server_redirect_map_[::std::string(url)] = ::std::string(redirect_url);
}

// 移除服务器 URL 重定向
void my::HttpRouterGuard::remove_redirect(::std::string_view url)
{
    server_redirect_map_.erase(::std::string(url));
}

// 检查服务器 URL 是否被阻止或重定向
::my::HttpRouterGuard::Response my::HttpRouterGuard::check_server(::std::string_view url) const
{
    if (server_blocked_.contains(::std::string(url))) {
        return Response::BLOCKED; // 如果在被阻止的服务器列表中，则返回 BLOCKED
    }
    if (server_redirect_map_.contains(::std::string(url))) {
        return Response::REDIRECTED; // 如果在重定向映射中，则返回 REDIRECTED
    }
    return Response::OK; // 否则返回 OK
}

// 获取服务器 URL 的重定向地址
::std::string my::HttpRouterGuard::get_redirect_url(::std::string_view url) const
{
    return server_redirect_map_.at(::std::string(url));
}