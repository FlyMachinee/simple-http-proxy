#include "../include/HttpCacheManager.h"
#include "../include/format_log.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

my::HttpCacheManager::HttpCacheManager(::std::string_view cache_dir) : cache_dir_(cache_dir)
{
    cache_time_map_filename_ = cache_dir_ + "\\cache_time_map";
    if (!::std::filesystem::exists(cache_dir_)) {
        ::std::filesystem::create_directory(cache_dir_);
    }

    ::std::ifstream ifs(cache_time_map_filename_);
    if (ifs.is_open()) {
        ::std::string key, time, etag, line;
        while (::std::getline(ifs, line)) {
            key = line;
            ::std::getline(ifs, line);
            time = line;
            ::std::getline(ifs, line);
            etag = line;
            if (::std::filesystem::exists(cache_dir_ + "\\" + key)) {
                cache_time_map_[key] = time;
                cache_etag_map_[key] = etag;
            }
        }
        ifs.close();
    }
}

my::HttpCacheManager::~HttpCacheManager()
{
    ::std::ofstream ofs(cache_time_map_filename_);
    if (!ofs.is_open()) {
        err("Failed to save cache_time_map file: " + cache_time_map_filename_);
        return;
    }
    for (auto &pair : cache_time_map_) {
        ofs << pair.first << ::std::endl;
        ofs << pair.second << ::std::endl;
        ofs << cache_etag_map_[pair.first] << ::std::endl;
    }
    ofs.close();
}

bool my::HttpCacheManager::has_cache(::std::string_view url) const
{
    ::std::lock_guard<::std::mutex> lock(cache_mutex_);
    return cache_time_map_.contains(get_key(url));
}

int my::HttpCacheManager::read_cache(::std::string_view url, char *buffer, int buf_size, int start) const
{
    ::std::lock_guard<::std::mutex> lock(cache_mutex_);

    ::std::string key = get_key(url);
    ::std::ifstream ifs(cache_dir_ + "\\" + key, ::std::ios::binary);
    if (ifs.is_open()) {
        ifs.seekg(start, ::std::ios::beg);
        ifs.read(buffer, buf_size);
        auto read_size = ifs.gcount();
        buffer[read_size] = '\0';
        ifs.close();
        return read_size;
    } else {
        throw ::std::runtime_error("Failed to open cache file: " + key + "(" + ::std::string(url) + ")");
    }
}

void my::HttpCacheManager::append_cache(::std::string_view url, const char *data, int data_size)
{
    ::std::lock_guard<::std::mutex> lock(cache_mutex_);

    ::std::string key = get_key(url);
    ::std::ofstream ofs(cache_dir_ + "\\" + key, ::std::ios::binary | ::std::ios::app);
    if (!ofs.is_open()) {
        throw ::std::runtime_error("Failed to open cache file: " + key + "(" + ::std::string(url) + ")");
    }
    ofs.write(data, data_size);
    ofs.close();
}

void my::HttpCacheManager::create_cache(::std::string_view url)
{
    ::std::lock_guard<::std::mutex> lock(cache_mutex_);

    ::std::string key = get_key(url);
    ::std::ofstream ofs(cache_dir_ + "\\" + key, ::std::ios::binary);
    if (!ofs.is_open()) {
        throw ::std::runtime_error("Failed to create cache file: " + key + "(" + ::std::string(url) + ")");
    }
    ofs.close();
}

bool my::HttpCacheManager::update_cache_time(::std::string_view url)
{
    ::std::unique_lock<::std::mutex> lock(cache_mutex_);

    ::std::string key = get_key(url);
    ::std::ifstream ifs(cache_dir_ + "\\" + key);
    if (!ifs.is_open()) {
        throw ::std::runtime_error("Failed to open cache file: " + key + "(" + ::std::string(url) + ")");
    }
    ::std::string line;
    bool find_last_modified = false, find_e_tag = false;
    while (::std::getline(ifs, line)) {
        if (line.empty()) {
            break;
        }
        if (line.find("Last-Modified:") == 0) {
            find_last_modified = true;
            cache_time_map_[key] = line.substr(15);
        } else if (line.find("ETag:") == 0) {
            find_e_tag = true;
            cache_etag_map_[key] = line.substr(6);
        }
    }
    ifs.close();
    if (find_last_modified || find_e_tag) {
        if (!find_last_modified) {
            cache_time_map_[key] = "";
        }
        if (!find_e_tag) {
            cache_etag_map_[key] = "";
        }
        return true;
    }
    lock.unlock();

    remove_cache(url);
    return false;
}

void my::HttpCacheManager::remove_cache(::std::string_view url)
{
    ::std::lock_guard<::std::mutex> lock(cache_mutex_);
    ::std::string key = get_key(url);
    ::std::filesystem::remove(cache_dir_ + "\\" + key);
    cache_time_map_.erase(key);
    cache_etag_map_.erase(key);
}

::std::string my::HttpCacheManager::get_modified_time(::std::string_view url) const
{
    ::std::lock_guard<::std::mutex> lock(cache_mutex_);
    return cache_time_map_.at(get_key(url));
}

::std::string my::HttpCacheManager::get_etag(::std::string_view url) const
{
    ::std::lock_guard<::std::mutex> lock(cache_mutex_);
    return cache_etag_map_.at(get_key(url));
}

::std::string my::HttpCacheManager::get_key(::std::string_view url)
{
    ::std::string host, path;
    size_t pos = 0;
    if (url.find("http://") == 0) {
        pos = 7;
    } else if (url.find("https://") == 0) {
        pos = 8;
    } else {
        return "";
    }

    size_t host_end = url.find('/', pos);
    host = ::std::string(url.substr(pos, host_end - pos));
    path = ::std::string(url.substr(host_end));

    ::std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
       << ::std::hash<::std::string>()(host)
       << ::std::hash<::std::string>()(path);
    return ss.str();
}
