#include "../include/HttpResponseHead.h"

#include <cstring>

// 构造函数，从原始 HTTP 数据初始化
my::HttpResponseHead::HttpResponseHead(const char *http_data, int size)
{
    this->set_all(http_data, size);
}

// 设置所有头部字段，从原始 HTTP 数据初始化
void my::HttpResponseHead::set_all(const char *http_data, int size)
{
    const char *line_start = http_data; // 当前行的起始位置
    const char *line_end;               // 当前行的结束位置

    // 解析状态行
    line_end = strchr(line_start, '\n');                            // 查找行结束符
    ::std::string_view line(line_start, line_end - line_start - 1); // 获取当前行

    // 解析 HTTP 版本
    ::std::string_view version = line.substr(0, ::std::min(line.find(' '), line.find('\r')));
    this->version = ::std::string(version);
    line.remove_prefix(version.size() + 1);

    // 解析状态码
    ::std::string_view status = line.substr(0, ::std::min(line.find(' '), line.find('\r')));
    this->status = ::std::string(status);
    line.remove_prefix(status.size() + 1);

    // 解析状态消息
    this->message = ::std::string(line);

    line_start = line_end + 1; // 移动到下一行

    // 解析头部字段
    while (!isspace(*line_start)) {
        line_end = strchr(line_start, '\n');                            // 查找行结束符
        ::std::string_view line(line_start, line_end - line_start - 1); // 获取当前行
        if (line.empty()) {
            break; // 如果行为空，则结束解析
        }

        // 解析头部字段名和值
        ::std::string_view key = line.substr(0, line.find(':')); // 获取头部字段名
        line.remove_prefix(key.size() + 2);
        ::std::string_view value = line; // 获取头部字段值
        this->headers[::std::string(key)] = ::std::string(value);

        line_start = line_end + 1; // 移动到下一行
    }
}