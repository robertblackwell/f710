#ifndef H_thread_safe_queue_h
#define H_thread_safe_queue_h

#include <thread>
#include <mutex>
#include <queue>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <condition_variable>
#include "std_format.h"
#include <functional>
#include <type_traits>
#include <sys/eventfd.h>
#include <fcntl.h>

// #include "fd_functions.h"
#define QUEUE_PIPE
namespace threadsafe {

inline int fd_set_non_blocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    int modFlags2 = flags | O_NONBLOCK;
    int fres = fcntl(fd, F_SETFL, modFlags2);
    if( fres < 0 ){
        int saved_errno = errno;
        throw std::runtime_error(std_format("fd_set_non_blocking failed errno: %d  strerror: %s", saved_errno, ::strerror(saved_errno)));
    }
    return fd;
}


    template<typename T>
    requires std::is_assignable_v<T&, T&&>
    class QueueInterface 
    {
        virtual void put(T data) = 0;
        virtual void get(T &data) = 0;
        virtual bool get_nowait(T &data) = 0; 
        virtual bool empty() = 0; 
    };


    /**
     * This is a simple threadsafe queue for passing values between threads.
     *
     *
     *
     * @tparam T
     */
    template<typename T>
    requires std::is_assignable_v<T&, T&&>
    class Queue 
    {
        std::queue<T> m_queue;
        std::mutex m_queue_mutex;
        std::condition_variable m_queue_cond_var;
    public:
        Queue() = default;
        ~Queue() = default;

        std::size_t put(T&& data) {
            std::size_t how_many;
            {
                // printf("Queue::put data: %s\n", data.c_str());
                std::lock_guard<std::mutex> lock{m_queue_mutex};
                m_queue.push(std::move(data));
                how_many = m_queue.size();
                m_queue_cond_var.notify_one();
            }
            return how_many;
        }

        void get(T &data) {
            {
                std::unique_lock<std::mutex> lock{m_queue_mutex};
                m_queue_cond_var.wait(lock, [this]() {
                    return ((!m_queue.empty()) && (m_queue.size() > 0));
                });
                data = std::move(m_queue.front());

                m_queue.pop();
            }
        }

        bool get_nowait(T &data) {
            {
                std::unique_lock<std::mutex> lock{m_queue_mutex};
                if ((!m_queue.empty()) && (m_queue.size() > 0)) {
                    data = std::move(m_queue.front());
                    m_queue.pop();
                    // printf("Queue::get_nowait NOT empty data:%s\n", data.c_str());
                    return true;
                }
            }
            return false;
        }

        bool empty() {
            {
                std::unique_lock<std::mutex> lock{m_queue_mutex};
                if ((!m_queue.empty()) && (m_queue.size() > 0)) {
                    return false;
                }
                return true;
            }
        }
    };

    template<typename T>
    requires std::is_assignable_v<T&, T&&>
    class FdQueue  
    {

    private:
        int m_write_fd;
        int m_read_fd;
        Queue<T> m_threadsafe_queue;
    public:
        FdQueue()
        {
#if defined(QUEUE_PIPE)
            int p[2];
            if(pipe(p) < 0) {
                throw std::runtime_error("pipe call failed");
            }
            m_write_fd = fd_set_non_blocking(p[1]);
            m_read_fd  = fd_set_non_blocking(p[0]);
#else
            int flags = EFD_CLOEXEC | EFD_NONBLOCK | EFD_SEMAPHORE;
            int fd = eventfd(0, flags);
            if(fd < 0) {
                throw std::runtime_error("eventfd call failed");
            }
            m_write_fd = fd;
            m_read_fd = fd;
#endif
        }
        ~FdQueue()
        {
            close(m_write_fd);
            close(m_read_fd);
        }
        bool empty()
        {
            return m_threadsafe_queue.empty();
        }
        void put(T&& t)
        {
            [[maybe_unused]]std::size_t qn = m_threadsafe_queue.put(std::move(t));
#if defined(QUEUE_PIPE)
            // const char* junk = "X";
            int n = write(m_write_fd, "X", 1);
            if(n != 1) {
                throw std::runtime_error("fdqueue put failed ");
            }
#else
            std::uint64_t v = 1;
            int n = write(this->write_fileno(), &v, sizeof(v));
            int saved_errno = errno;
            if(n < 0 && saved_errno != EAGAIN) {
                throw std::runtime_error(std_format("put failed n: %d errno %s", n, saved_errno));
            }
#endif
        }

        void get(T& t)
        {
            char buf[10];
            [[maybe_unused]]ssize_t n = read(m_read_fd, buf, 1);
            m_threadsafe_queue.get(t);
        }
        
        bool get_nowait(T& t)
        {
#if defined(QUEUE_PIPE)
            char buf[10];
            [[maybe_unused]]ssize_t n = read(m_read_fd, buf, 1);
#else
            std::uint64_t v;
            int n = read(this->read_fileno(), &v, sizeof(v));
#endif
            return m_threadsafe_queue.get_nowait(t);
        }
        int read_fileno()
        {
            return m_read_fd;
        }

        int write_fileno()
        {
            return m_write_fd;
        }
    };

    template<typename T>
    requires std::is_assignable_v<T&, T&&>
    class TriggerQueue  
    {
        // typedef void(*trigger_func_t)();
        using trigger_func_t = std::function<void()>;
    private:
        trigger_func_t m_trigger_func;
        Queue<T> m_threadsafe_queue;
    public:
        explicit TriggerQueue( std::function<void()> trigger_func)
        {
            m_trigger_func = trigger_func;
        }
        bool empty()
        {
            return m_threadsafe_queue.empty();
        }
        std::size_t put(T&& t)
        {
            std::size_t n = m_threadsafe_queue.put(std::move(t));
            m_trigger_func();
            return n;
        }
        void get(T& t)
        {
            m_threadsafe_queue.get(t);
        }
        bool get_nowait(T& t)
        {
            return m_threadsafe_queue.get_nowait(t);
        }
    };

} //namespace
#endif


