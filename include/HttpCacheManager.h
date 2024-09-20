#ifndef _HTTP_CACHE_MANAGER_H_INCLUDED_
#define _HTTP_CACHE_MANAGER_H_INCLUDED_

#include "./HttpRequest.h"
#include "./HttpResponseHead.h"
#include <map>
#include <mutex>
#include <string>

namespace my
{

    class HttpCacheManager
    {
    public:
        HttpCacheManager(::std::string_view cache_dir);
        ~HttpCacheManager();

        bool has_cache(::std::string_view url) const;
        int read_cache(::std::string_view url, char *buffer, int buf_size, int start) const;
        void append_cache(::std::string_view url, const char *data, int data_size);

        void create_cache(::std::string_view url);
        bool update_cache_time(::std::string_view url);
        void remove_cache(::std::string_view url);

        ::std::string get_modified_time(::std::string_view url) const;
        ::std::string get_etag(::std::string_view url) const;

        static ::std::string get_key(::std::string_view url);

        HttpCacheManager(const HttpCacheManager &) = delete;
        HttpCacheManager &operator=(const HttpCacheManager &) = delete;
        HttpCacheManager(HttpCacheManager &&) = delete;
        HttpCacheManager &operator=(HttpCacheManager &&) = delete;

    private:
        ::std::string cache_dir_;
        ::std::string cache_time_map_filename_;
        ::std::map<::std::string, ::std::string> cache_time_map_;
        ::std::map<::std::string, ::std::string> cache_etag_map_;

        mutable ::std::mutex cache_mutex_;
    }; // class CacheManager

} // namespace my

#endif // _HTTP_CACHE_MANAGER_H_INCLUDED_