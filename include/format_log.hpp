#ifndef _FORMAT_LOG_HPP_INCLUDED_
#define _FORMAT_LOG_HPP_INCLUDED_

#include <format>
#include <iostream>
#include <mutex>
#include <string_view>

namespace my
{
    // 全局互斥锁，用于保护日志输出
    inline ::std::mutex __log_mutex;

    // 格式化日志输出函数模板
    template <typename... Args>
    inline void format_log(
        ::std::ostream &os,        // 输出流
        int leading_space,         // 前导空格数量
        ::std::string_view ps,     // 前缀字符串
        ::std::string_view format, // 格式字符串
        Args &&...args)            // 可变参数
    {
        // 使用互斥锁保护日志输出
        ::std::lock_guard<::std::mutex> lock(__log_mutex);
        // 输出格式化后的日志信息
        os << ::std::string(leading_space, ' ') << ps << ::std::vformat(format, ::std::make_format_args(args...)) << '\n';
    }

    // 输出到标准输出流的函数模板，带前导空格
    template <typename... Args>
    inline void out(int leading_space, ::std::string_view format, Args &&...args)
    {
        format_log(::std::cout, leading_space, "", format, ::std::forward<Args>(args)...);
    }

    // 输出到标准输出流的函数模板，不带前导空格
    template <typename... Args>
    inline void out(::std::string_view format, Args &&...args)
    {
        format_log(::std::cout, 0, "", format, ::std::forward<Args>(args)...);
    }

    // 输出到标准日志流的函数模板，带 [log] 前缀
    template <typename... Args>
    inline void log(::std::string_view format, Args &&...args)
    {
        format_log(::std::clog, 0, "[log] ", format, ::std::forward<Args>(args)...);
    }

    // 输出到标准错误流的函数模板，带 [error] 前缀
    template <typename... Args>
    inline void err(::std::string_view format, Args &&...args)
    {
        format_log(::std::cerr, 0, "[error] ", format, ::std::forward<Args>(args)...);
    }

    // 输出到标准输出流的函数模板，带指定数量的前导空格
    template <int N, typename... Args>
    inline void con(::std::string_view format, Args &&...args)
    {
        format_log(::std::cout, N, "", format, ::std::forward<Args>(args)...);
        // N 是用于控制输出缩进的参数，对于 log 是 6，对于 error 是 8
    }

} // namespace my

#endif // _FORMAT_LOG_HPP_INCLUDED_