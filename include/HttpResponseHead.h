#ifndef _HTTP_RESPONSE_HEAD_H_INCLUDED_
#define _HTTP_RESPONSE_HEAD_H_INCLUDED_

#include <map>
#include <string>

namespace my
{
    struct HttpResponseHead {
        ::std::string version;
        ::std::string status;
        ::std::string message;

        ::std::map<::std::string, ::std::string> headers;

        HttpResponseHead() = default;
        HttpResponseHead(const char *http_data, int size);
        ~HttpResponseHead() = default;

        void set_all(const char *http_data, int size);
    };

} // namespace my

#endif // _HTTP_RESPONSE_HEAD_H_INCLUDED_