#include "../include/HttpRouterGuard.h"

void ::my::HttpRouterGuard::add_client(::std::string_view ip)
{
    client_blocked_.insert(::std::string(ip));
}

void ::my::HttpRouterGuard::remove_client(::std::string_view ip)
{
    client_blocked_.erase(::std::string(ip));
}

::my::HttpRouterGuard::Response my::HttpRouterGuard::check_client(::std::string_view ip) const
{
    if (client_blocked_.find(::std::string(ip)) != client_blocked_.end()) {
        return Response::BLOCKED;
    }
    return Response::OK;
}

void ::my::HttpRouterGuard::add_server(::std::string_view url)
{
    server_blocked_.insert(::std::string(url));
}

void ::my::HttpRouterGuard::remove_server(::std::string_view url)
{
    server_blocked_.erase(::std::string(url));
}

void my::HttpRouterGuard::add_redirect(::std::string_view url, ::std::string_view redirect_url)
{
    server_redirect_map_[::std::string(url)] = ::std::string(redirect_url);
}

void my::HttpRouterGuard::remove_redirect(::std::string_view url)
{
    server_redirect_map_.erase(::std::string(url));
}

::my::HttpRouterGuard::Response my::HttpRouterGuard::check_server(::std::string_view url) const
{
    if (server_blocked_.contains(::std::string(url))) {
        return Response::BLOCKED;
    }
    if (server_redirect_map_.contains(::std::string(url))) {
        return Response::REDIRECTED;
    }
    return Response::OK;
}

::std::string my::HttpRouterGuard::get_redirect_url(::std::string_view url) const
{
    return server_redirect_map_.at(::std::string(url));
}