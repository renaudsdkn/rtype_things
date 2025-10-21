#pragma once
#include <queue>
#include <mutex>
#include <memory>
#include <vector>
#include <asio.hpp>

struct NetworkPacket {
    asio::ip::udp::endpoint sender;
    std::vector<uint8_t> data;
};

template<typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(value));
    }
    std::queue<T> popAll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<T> localQueue;
        std::swap(localQueue, m_queue);
        return localQueue;
    }
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
};