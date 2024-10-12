#ifndef _HTTP_PROXY_SERVER_H_INCLUDED_
#define _HTTP_PROXY_SERVER_H_INCLUDED_

#include "./Host.h"
#include "./HttpCacheManager.h"
#include "./HttpRequest.h"
#include "./HttpRouterGuard.h"
#include "./SimpleThreadPool.hpp"
#include <atomic>
#include <windows.h>
#include <winsock2.h>

namespace my
{
    // CheckCacheResult 枚举表示缓存检查的结果
    enum class CheckCacheResult {
        NONE,          // 无缓存
        NOT_SUPPORTED, // 不支持缓存
        NO_CACHE,      // 没有缓存
        FOUND,         // 找到缓存
        EXPIRED,       // 缓存已过期
    };

    // HttpProxyServer 类用于实现 HTTP 代理服务器
    class HttpProxyServer
    {
    public:
        static constexpr int MAX_BUFFER_SIZE = 65535; // 最大缓冲区大小

        // 构造函数，初始化代理服务器
        HttpProxyServer(const char *p_ip, unsigned short p_port, bool use_cache = false);
        // 析构函数，清理资源
        ~HttpProxyServer();

        // 运行代理服务器
        bool run();
        // 运行多线程代理服务器
        bool run_multithread();
        // 检查代理服务器是否正在运行
        bool is_running() const;

        // 获取路由守护对象
        HttpRouterGuard &router_guard();

        // 禁用拷贝构造函数
        HttpProxyServer(const HttpProxyServer &) = delete;
        // 禁用拷贝赋值运算符
        HttpProxyServer &operator=(const HttpProxyServer &) = delete;
        // 禁用移动构造函数
        HttpProxyServer(HttpProxyServer &&) = delete;
        // 禁用移动赋值运算符
        HttpProxyServer &operator=(HttpProxyServer &&) = delete;

    private:
        // 内部运行方法，支持单线程和多线程
        bool inner_run(bool is_multithread);
        // 处理客户端请求
        void handle_client(int c_no, Host client);

        // 检查缓存并接收数据
        CheckCacheResult check_cache_and_recv(HttpRequest client_request, const Host &server, char *buffer, int buf_size, int &recv_size);
        // 从缓存中响应请求
        int answer_from_cache(::std::string_view url, const Host &client);
        // 从服务器响应请求
        int answer_from_server(CheckCacheResult chk_res, ::std::string_view url, const Host &client, const Host &server, char *buffer, int buf_size, int recv_size);

        // 发送请求并接收响应
        int send_and_recv(const HttpRequest &request, const Host &host, char *recv_buffer, int buf_size);

        int p_no_;                       // 代理服务器编号
        Host proxy_;                     // 代理服务器主机信息
        bool use_cache_;                 // 是否使用缓存
        HttpCacheManager cache_manager_; // 缓存管理器
        HttpRouterGuard router_guard_;   // 路由守护对象
        ::std::atomic_int task_count_;   // 任务计数
        ::std::atomic_bool is_running_;  // 运行状态

        SimpleThreadPool thread_pool_; // 线程池

        static int instance_count_; // 实例计数
        static int p_id_;           // 代理服务器 ID
    }; // class HttpProxyServer

} // namespace my

#endif // _HTTP_PROXY_SERVER_H_INCLUDED_