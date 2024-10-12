#ifndef _HTTP_REQUEST_H_INCLUDED_
#define _HTTP_REQUEST_H_INCLUDED_

#include <map>
#include <string>

namespace my
{
    // HttpRequest 结构体表示一个 HTTP 请求
    struct HttpRequest {
        ::std::string method;  // HTTP 方法（如 GET, POST）
        ::std::string url;     // 请求的 URL
        ::std::string version; // HTTP 版本（如 HTTP/1.1）

        ::std::map<std::string, std::string> headers; // 请求头部字段
        ::std::string body;                           // 请求体

        // 默认构造函数
        HttpRequest() = default;
        // 构造函数，从原始 HTTP 数据初始化
        HttpRequest(const char *http_data, int size);
        // 默认析构函数
        ~HttpRequest() = default;

        // 获取主机和端口号
        ::std::pair<::std::string, unsigned short> get_host_port() const;
        // 将请求转换为字符串
        ::std::string to_string() const;
    };
} // namespace my

#endif // _HTTP_REQUEST_H_INCLUDED_