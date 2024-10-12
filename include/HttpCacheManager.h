#ifndef _HTTP_CACHE_MANAGER_H_INCLUDED_
#define _HTTP_CACHE_MANAGER_H_INCLUDED_

#include "./HttpRequest.h"
#include "./HttpResponseHead.h"
#include <map>
#include <mutex>
#include <string>

namespace my
{

    // HttpCacheManager 类用于管理 HTTP 缓存
    class HttpCacheManager
    {
    public:
        // 构造函数，接受缓存目录路径
        HttpCacheManager(::std::string_view cache_dir);
        // 析构函数
        ~HttpCacheManager();

        // 检查指定 URL 是否有缓存
        bool has_cache(::std::string_view url) const;
        // 读取指定 URL 的缓存数据
        int read_cache(::std::string_view url, char *buffer, int buf_size, int start) const;
        // 追加数据到指定 URL 的缓存
        void append_cache(::std::string_view url, const char *data, int data_size);

        // 创建指定 URL 的缓存
        void create_cache(::std::string_view url);
        // 更新指定 URL 的缓存时间
        bool update_cache_time(::std::string_view url);
        // 移除指定 URL 的缓存
        void remove_cache(::std::string_view url);

        // 获取指定 URL 的最后修改时间
        ::std::string get_modified_time(::std::string_view url) const;
        // 获取指定 URL 的 ETag
        ::std::string get_etag(::std::string_view url) const;

        // 获取指定 URL 的缓存键
        static ::std::string get_key(::std::string_view url);

        // 禁用拷贝构造函数
        HttpCacheManager(const HttpCacheManager &) = delete;
        // 禁用拷贝赋值运算符
        HttpCacheManager &operator=(const HttpCacheManager &) = delete;
        // 禁用移动构造函数
        HttpCacheManager(HttpCacheManager &&) = delete;
        // 禁用移动赋值运算符
        HttpCacheManager &operator=(HttpCacheManager &&) = delete;

    private:
        // 缓存目录路径
        ::std::string cache_dir_;
        // 缓存时间映射文件名
        ::std::string cache_time_map_filename_;
        // 缓存时间映射
        ::std::map<::std::string, ::std::string> cache_time_map_;
        // 缓存 ETag 映射
        ::std::map<::std::string, ::std::string> cache_etag_map_;

        // 用于保护缓存数据的互斥锁
        mutable ::std::mutex cache_mutex_;
    }; // class CacheManager

} // namespace my

#endif // _HTTP_CACHE_MANAGER_H_INCLUDED_