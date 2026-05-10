#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

/**
 * 线程安全队列模板类
 * 
 * 实现了经典的生产者-消费者模式，支持阻塞式的 push 和 pop。
 */
template<typename T>
class SafeQueue {
public:
    SafeQueue(size_t max_size = 0) : max_size_(max_size), abort_(false) {}

    ~SafeQueue() {
        abort();
        clear();
    }

    // 入队：如果队列已满，则阻塞直到有空间
    bool push(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_full_.wait(lock, [this] { return abort_ || max_size_ == 0 || queue_.size() < max_size_; });
        
        if (abort_) return false;

        queue_.push(std::move(value));
        cond_empty_.notify_one();
        return true;
    }

    // 出队：如果队列为空，则阻塞直到有数据
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_empty_.wait(lock, [this] { return abort_ || !queue_.empty(); });

        if (abort_ && queue_.empty()) return false;

        value = std::move(queue_.front());
        queue_.pop();
        cond_full_.notify_one();
        return true;
    }

    // 唤醒所有等待中的线程并标记为停止
    void abort() {
        abort_ = true;
        cond_empty_.notify_all();
        cond_full_.notify_all();
    }

    // 清空队列并返回所有剩余项，以便外部释放内存
    std::vector<T> flush() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<T> remaining;
        while (!queue_.empty()) {
            remaining.push_back(std::move(queue_.front()));
            queue_.pop();
        }
        cond_full_.notify_all(); // 通知可能阻塞在 push 的线程
        return remaining;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            // 重要：如果 T 是指针类型（如 AVPacket* 或 AVFrame*），
            // 本方法仅清空队列容器，并不会调用 av_packet_free 等释放逻辑。
            // 外部必须通过循环 pop 并手动释放来清理资源。
            queue_.pop();
        }
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_empty_;
    std::condition_variable cond_full_;
    size_t max_size_;
    std::atomic<bool> abort_;
};

#endif // SAFE_QUEUE_H
