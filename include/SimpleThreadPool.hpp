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
    class SimpleThreadPool
    {
    public:
        using TaskType = ::std::function<void()>;

        SimpleThreadPool(int thread_count) : threads_(thread_count), is_running_(true)
        {
            initialize();
        }

        SimpleThreadPool() : SimpleThreadPool(::std::thread::hardware_concurrency()) {}

        ~SimpleThreadPool()
        {
            if (is_running_) {
                stop();
            }
        }

        template <typename F, typename... Args>
        auto add_task(F &&f, Args &&...args)
        {
            using ReturnType = ::std::invoke_result_t<F, Args...>;
            auto task = ::std::make_shared<::std::packaged_task<ReturnType()>>(
                ::std::bind(::std::forward<F>(f), ::std::forward<Args>(args)...));
            {
                ::std::lock_guard<::std::mutex> lock(mutex_);
                tasks_queue_.push([task]() { (*task)(); });
            }
            task_cv_.notify_one();
            return task->get_future();
        }

        void wait_all()
        {
            while (!tasks_queue_.empty()) {
                ::std::this_thread::yield();
            }
        }

        void stop()
        {
            is_running_ = false;
            task_cv_.notify_all();
            for (auto &thread : threads_) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }

        SimpleThreadPool(const SimpleThreadPool &) = delete;
        SimpleThreadPool &operator=(const SimpleThreadPool &) = delete;
        SimpleThreadPool(SimpleThreadPool &&) = delete;
        SimpleThreadPool &operator=(SimpleThreadPool &&) = delete;

    private:
        void initialize()
        {
            for (auto &thread : threads_) {
                auto worker = [this]() {
                    while (is_running_) {
                        TaskType task;
                        {
                            ::std::unique_lock<::std::mutex> lock(mutex_);
                            task_cv_.wait(lock, [this]() { return !tasks_queue_.empty() || !is_running_; });
                            if (!is_running_) {
                                return;
                            }
                            task = ::std::move(tasks_queue_.front());
                            tasks_queue_.pop();
                        }
                        task();
                    }
                };
            }
        }

        ::std::vector<::std::thread> threads_;
        ::std::queue<TaskType> tasks_queue_;
        ::std::mutex mutex_;
        ::std::condition_variable task_cv_;
        ::std::atomic_bool is_running_;
    };
} // namespace my

#endif // _SIMPLE_THREAD_POOL_H_INCLUDED_