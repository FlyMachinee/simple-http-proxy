#include "../include/HttpProxyServer.h"
#include <iostream>

int main(int argc, char const *argv[])
{
    ::my::HttpProxyServer proxy("127.0.0.1", 1920, true);
    proxy.router_guard().add_redirect("http://www.1920.com/", "http://today.hit.edu.cn/");
    proxy.router_guard().add_server("http://jwts.hit.edu.cn/");
    proxy.router_guard().add_client("127.0.0.1");

    proxy.run();
    ::std::cout.flush();
    ::std::clog.flush();
    return 0;
}