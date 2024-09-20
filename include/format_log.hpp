#ifndef _FORMAT_LOG_HPP_INCLUDED_
#define _FORMAT_LOG_HPP_INCLUDED_

#include <format>
#include <iostream>
#include <string_view>

#include <mutex>

namespace my
{
    inline ::std::mutex __log_mutex;

    template <typename... Args>
    inline void format_log(
        ::std::ostream &os,
        int leading_space,
        ::std::string_view ps,
        ::std::string_view format,
        Args &&...args)
    {
        ::std::lock_guard<::std::mutex> lock(__log_mutex);
        os << ::std::string(leading_space, ' ') << ps << ::std::vformat(format, ::std::make_format_args(args...)) << '\n';
    }

    template <typename... Args>
    inline void out(int leading_space, ::std::string_view format, Args &&...args)
    {
        format_log(::std::cout, leading_space, "", format, ::std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void out(::std::string_view format, Args &&...args)
    {
        format_log(::std::cout, 0, "", format, ::std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void log(::std::string_view format, Args &&...args)
    {
        format_log(::std::clog, 0, "[log] ", format, ::std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void err(::std::string_view format, Args &&...args)
    {
        format_log(::std::cerr, 0, "[error] ", format, ::std::forward<Args>(args)...);
    }

    template <int N, typename... Args>
    inline void con(::std::string_view format, Args &&...args)
    {
        format_log(::std::cout, N, "", format, ::std::forward<Args>(args)...);
        // N is 6 for log, and 8 for error
    }

} // namespace my

#endif // _FORMAT_LOG_HPP_INCLUDED_