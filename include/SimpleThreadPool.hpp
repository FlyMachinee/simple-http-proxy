#ifndef _SIMPLE_THREAD_POOL_H_INCLUDED_
#define _SIMPLE_THREAD_POOL_H_INCLUDED_

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace my
{
    // SimpleThreadPool 类用于管理一个简单的线程池
    class SimpleThreadPool
    {
    public:
        using TaskType = ::std::function<void()>; // 任务类型定义

        // 构造函数，接受线程数量参数
        SimpleThreadPool(int thread_count) : threads_(thread_count), is_running_(true)
        {
            initialize();
        }

        // 默认构造函数，使用硬件并发线程数
        SimpleThreadPool() : SimpleThreadPool(::std::thread::hardware_concurrency()) {}

        // 析构函数，停止线程池
        ~SimpleThreadPool()
        {
            if (is_running_) {
                stop();
            }
        }

        // 添加任务到线程池
        template <typename F, typename... Args>
        auto add_task(F &&f, Args &&...args)
        {
            using ReturnType = ::std::invoke_result_t<F, Args...>; // 任务返回类型
            auto task = ::std::make_shared<::std::packaged_task<ReturnType()>>(
                ::std::bind(::std::forward<F>(f), ::std::forward<Args>(args)...)); // 创建任务
            {
                ::std::lock_guard<::std::mutex> lock(mutex_); // 加锁
                tasks_queue_.push([task]() { (*task)(); });   // 将任务添加到队列
            }
            task_cv_.notify_one();     // 通知一个等待线程
            return task->get_future(); // 返回任务的 future 对象
        }

        // 等待所有任务完成
        void wait_all()
        {
            while (!tasks_queue_.empty()) {
                ::std::this_thread::yield(); // 让出 CPU
            }
        }

        // 停止线程池
        void stop()
        {
            is_running_ = false;   // 设置运行标志为 false
            task_cv_.notify_all(); // 通知所有等待线程
            for (auto &thread : threads_) {
                if (thread.joinable()) {
                    thread.join(); // 等待线程结束
                }
            }
        }

        // 禁用拷贝构造函数
        SimpleThreadPool(const SimpleThreadPool &) = delete;
        // 禁用拷贝赋值运算符
        SimpleThreadPool &operator=(const SimpleThreadPool &) = delete;
        // 禁用移动构造函数
        SimpleThreadPool(SimpleThreadPool &&) = delete;
        // 禁用移动赋值运算符
        SimpleThreadPool &operator=(SimpleThreadPool &&) = delete;

    private:
        // 初始化线程池
        void initialize()
        {
            for (auto &thread : threads_) {
                auto worker = [this]() {
                    while (is_running_) {
                        TaskType task;
                        {
                            ::std::unique_lock<::std::mutex> lock(mutex_);                                   // 加锁
                            task_cv_.wait(lock, [this]() { return !tasks_queue_.empty() || !is_running_; }); // 等待任务或停止信号
                            if (!is_running_) {
                                return; // 如果停止则退出
                            }
                            task = ::std::move(tasks_queue_.front()); // 获取任务
                            tasks_queue_.pop();                       // 移除任务
                        }
                        task(); // 执行任务
                    }
                };
                thread = ::std::thread(worker); // 创建并启动线程
            }
        }

        ::std::vector<::std::thread> threads_; // 线程池
        ::std::queue<TaskType> tasks_queue_;   // 任务队列
        ::std::mutex mutex_;                   // 互斥锁
        ::std::condition_variable task_cv_;    // 条件变量
        ::std::atomic_bool is_running_;        // 运行标志
    };
} // namespace my

#endif // _SIMPLE_THREAD_POOL_H_INCLUDED_