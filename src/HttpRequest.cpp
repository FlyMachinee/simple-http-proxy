#include "../include/HttpRequest.h"

#include <cstring>

// 构造函数，从原始 HTTP 数据初始化
::my::HttpRequest::HttpRequest(const char *http_data, int size)
{
    const char *line_start = http_data; // 当前行的起始位置
    const char *line_end;               // 当前行的结束位置
    const char *body_start;             // 请求体的起始位置

    // 解析请求行和头部字段
    while (!isspace(*line_start)) {
        line_end = strchr(line_start, '\n');                            // 查找行结束符
        ::std::string_view line(line_start, line_end - line_start - 1); // 获取当前行
        if (line.empty()) {
            break; // 如果行为空，则结束解析
        }

        // 解析请求行
        if (this->method.empty()) {
            ::std::string_view method = line.substr(0, line.find(' ')); // 获取 HTTP 方法
            this->method = ::std::string(method);
            line.remove_prefix(method.size() + 1);

            ::std::string_view url = line.substr(0, line.find(' ')); // 获取 URL
            this->url = ::std::string(url);
            line.remove_prefix(url.size() + 1);

            this->version = ::std::string(line); // 获取 HTTP 版本
        } else {
            // 解析头部字段
            ::std::string_view key = line.substr(0, line.find(':')); // 获取头部字段名
            line.remove_prefix(key.size() + 2);
            ::std::string_view value = line; // 获取头部字段值
            this->headers[::std::string(key)] = ::std::string(value);
        }

        line_start = line_end + 1; // 移动到下一行
    }

    body_start = line_start + 2;                                             // 请求体的起始位置
    this->body = ::std::string(body_start, size - (body_start - http_data)); // 获取请求体
}

// 获取主机和端口号
::std::pair<::std::string, unsigned short> my::HttpRequest::get_host_port() const
{
    ::std::string host;
    unsigned short port = 80; // 默认端口号为 80

    auto it = this->headers.find("Host");
    if (it != this->headers.end()) {
        ::std::string_view host_port = it->second;
        auto pos = host_port.find(':');
        if (pos != ::std::string_view::npos) {
            host = host_port.substr(0, pos);                              // 获取主机名
            port = ::std::stoi(::std::string(host_port.substr(pos + 1))); // 获取端口号
        } else {
            host = ::std::string(host_port); // 仅获取主机名
        }
    }

    return {host, port};
}

// 将请求转换为字符串
::std::string my::HttpRequest::to_string() const
{
    ::std::string str = this->method + " " + this->url + " " + this->version + "\r\n";
    for (const auto &[key, value] : this->headers) {
        str += key + ": " + value + "\r\n";
    }
    str += "\r\n" + this->body;
    return str;
}