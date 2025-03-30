#ifndef DERRY_SAFE_QUEUE_H
#define DERRY_SAFE_QUEUE_H

// TODO 同学们注意：当前的文件，就是DerryPlayer播放器工程里面直接拿过来用的，只要是学过DerryPlayer的同学，是没有任何问题的

#include <queue>
#include <pthread.h>
#include <mutex>

using namespace std;
namespace live {
template <typename T>
class SafeQueue {
    typedef void (*ReleaseCallback)(T *);
    typedef void (*SyncHandle)(queue<T> &);

public:
    SafeQueue() {
    }

    ~SafeQueue() {
    }

    /**
     * 入队
     * @param value
     */
    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (work) {
            // 工作状态需要push
            q.push(value);
            condVar_.notify_one();
        } else {
            // 非工作状态
            if (releaseCallback) {
                releaseCallback(&value); // T无法释放， 让外界释放
            }
        }
    }

    /**
     * 出队
     * @param value
     * @return
     */
    int pop(T &value) {
        int ret = 0;
        std::unique_lock<std::mutex> lock(mutex_);
        condVar_.wait(lock, [&] {
            return work == 1 && q.size() != 0;
        });

        if (!q.empty()) {
            value = q.front();
            // 弹出
            q.pop();
            ret = 1;
        }
        return ret;
    }

    /**
     * 设置队列的工作状态
     * @param work
     */
    void setWork(int work) {
        std::lock_guard<std::mutex> lock(mutex_);
        this->work = work;
        condVar_.notify_one();
    }

    /**
     * 判断队列是否为空
     * @return
     */
//    int empty() {
//        return q.empty();
//    }

    /**
     * 获取队列大小
     * @return
     */
    int size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return q.size();
    }

    /**
     * 清空队列 队列中的元素如何释放？ 让外界释放
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        unsigned int size = q.size();
        for (int i = 0; i < size; ++i) {
            // 取出队首元素
            T value = q.front();
            if (releaseCallback) {
                releaseCallback(&value);
            }
            q.pop();
        }
    }

    void setReleaseCallback(ReleaseCallback releaseCallback) {
        this->releaseCallback = releaseCallback;
    }

    void setSyncHandle(SyncHandle syncHandle) {
        this->syncHandle = syncHandle;
    }
    /**
     * 同步操作
     */
    void sync() {
        std::lock_guard<std::mutex> lock(mutex_);
        syncHandle(q);
    }

private:
    queue<T> q;
    int work; // 标记队列是否工作
    ReleaseCallback releaseCallback;
    SyncHandle syncHandle;
    std::mutex mutex_;
    std::condition_variable condVar_;

};
} // namespace live

#endif // DERRY_SAFE_QUEUE_H
