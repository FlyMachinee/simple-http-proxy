#ifndef _HTTP_RESPONSE_HEAD_H_INCLUDED_
#define _HTTP_RESPONSE_HEAD_H_INCLUDED_

#include <map>
#include <string>

namespace my
{
    // HttpResponseHead 结构体表示一个 HTTP 响应头部
    struct HttpResponseHead {
        ::std::string version; // HTTP 版本（如 HTTP/1.1）
        ::std::string status;  // 响应状态码（如 200, 404）
        ::std::string message; // 响应状态消息（如 OK, Not Found）

        ::std::map<::std::string, ::std::string> headers; // 响应头部字段

        // 默认构造函数
        HttpResponseHead() = default;
        // 构造函数，从原始 HTTP 数据初始化
        HttpResponseHead(const char *http_data, int size);
        // 默认析构函数
        ~HttpResponseHead() = default;

        // 设置所有头部字段，从原始 HTTP 数据初始化
        void set_all(const char *http_data, int size);
    };

} // namespace my

#endif // _HTTP_RESPONSE_HEAD_H_INCLUDED_