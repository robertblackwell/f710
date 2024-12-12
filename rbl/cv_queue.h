#ifndef rbl_cv_QUEUE_H
#define rbl_cv_QUEUE_H


#include <thread>
#include <mutex>
#include <queue>
#include <iostream>
#include <condition_variable>


template<typename T>
class ConditionVariableQueue {
    std::queue<T> m_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cond_var;
    std::unique_ptr<std::thread> m_thread_object_uptr;
    bool m_stop_flag;
    long m_max_queue_size;
public:

    static std::shared_ptr<ConditionVariableQueue> make() {
        return std::make_shared<ConditionVariableQueue>();
    }

    ConditionVariableQueue() : m_max_queue_size(1000)
    {
        // std::set_terminate([](){
        //     std::cout << "Inside Writer::terminate" << std::endl;
        // });
        m_stop_flag = false;
        std::atexit([]() {
            std::cout << "Inside CVQueue::exit handler" << std::endl;
        });
    }

    ~ConditionVariableQueue() {
        std::cout << "worker destructor before wait"
                  << std::endl; //"queue size : " << m_logdata_queue.size() << std::endl;
        std::cout <<
                  "worker destructor after wait max queue size:  "
                  << m_max_queue_size
                  << " current queue size: " << m_queue.size()
                  << std::endl; //"queue size : " << m_logdata_queue.size() << std::endl;
    }

    void cleanup() {

    }

    void put(T item) {
        {
            std::lock_guard<std::mutex> lock{m_queue_mutex};
            m_queue.push(std::move(item));
            m_queue_cond_var.notify_one();
        }
    }
    void emplace(T item)
    {
        {
            std::lock_guard<std::mutex> lock{m_queue_mutex};
            m_queue.emplace(item);
            m_queue_cond_var.notify_one();
        }
    }

    T get_wait() {
        T ldata_sptr;
        {
            std::unique_lock<std::mutex> lock{m_queue_mutex};
            m_queue_cond_var.wait(lock, [this]() {
                return ((!m_queue.empty()) && (m_queue.size() > 0)) || (m_stop_flag);
            });
            ldata_sptr = std::move(m_queue.front());
            m_queue.pop();
        }
        return ldata_sptr;
    }

    std::size_t length() {
        int len;
        {
            std::unique_lock<std::mutex> lock{m_queue_mutex};
            m_queue_cond_var.wait(lock, [this]() {
                return ((!m_queue.empty()) && (m_queue.size() > 0)) || (m_stop_flag);
            });
            len = m_queue.size();
        }
        return len;
    }
};
#endif