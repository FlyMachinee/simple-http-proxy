#ifndef _HTTP_REQUEST_H_INCLUDED_
#define _HTTP_REQUEST_H_INCLUDED_

#include <map>
#include <string>

namespace my
{
    struct HttpRequest {
        ::std::string method;
        ::std::string url;
        ::std::string version;

        ::std::map<std::string, std::string> headers;
        ::std::string body;

        HttpRequest() = default;
        HttpRequest(const char *http_data, int size);
        ~HttpRequest() = default;

        ::std::pair<::std::string, unsigned short> get_host_port() const;
        ::std::string to_string() const;
    };
} // namespace my

#endif // _HTTP_REQUEST_H_INCLUDED_