#include "../include/HttpProxyServer.h"

int main(int argc, char const *argv[])
{
    ::my::HttpProxyServer proxy("127.0.0.1", 8080);
    proxy.run();
    return 0;
}