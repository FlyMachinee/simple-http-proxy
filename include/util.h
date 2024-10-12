#ifndef _UTIL_H_INCLUDED_
#define _UTIL_H_INCLUDED_

#include "../include/HttpRequest.h"

namespace my
{
    // 输出 HTTP 数据的函数
    // indent: 缩进级别
    // data: HTTP 数据
    // size: 数据大小
    void out_http_data(int indent, const char *data, int size);
} // namespace my

#endif // _UTIL_H_INCLUDED_