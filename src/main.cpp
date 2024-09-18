#include "../include/HttpProxyServer.h"
#include <iostream>

int main(int argc, char const *argv[])
{
    ::my::HttpProxyServer proxy("127.0.0.1", 1920, true);
    proxy.run();
    ::std::cout.flush();
    ::std::clog.flush();
    return 0;
}