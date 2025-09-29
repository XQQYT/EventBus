/*
 * EventBus
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 */

#ifndef THREADQUEUE_H
#define THREADQUEUE_H

#include <iostream>
#include <queue>
#include <mutex>
#include <atomic>
#include <shared_mutex>
#include <functional>
#include <tuple>
#include "Queue.h"

static const int default_capacity = 1024;

template <class... Args>
class ThreadQueue : public Queue<Args...> {
public:
    explicit ThreadQueue(int max) noexcept
        : capacity(max) 
    {
    }

    ThreadQueue() 
        : capacity(default_capacity)
    {
    }

    void addTask(std::function<void(Args...)>&& func, Args&&... args) {
        std::unique_lock<std::mutex> lock(mtx);
        if (task_queue.size() >= capacity) 
		{
            throw std::runtime_error("queue is full");
        }
        
        auto arg_tuple = std::make_tuple(std::forward<Args>(args)...);
        task_queue.push({std::move(func), std::move(arg_tuple)});
        size.fetch_add(1);
    }

    std::pair<std::function<void(Args...)>, std::tuple<Args...>> getTask() {
        std::lock_guard<std::mutex> lock(mtx);
        
        if (task_queue.empty()) 
		{
            throw std::runtime_error("task queue is empty");
        }
        
        auto task = std::move(task_queue.front());
        task_queue.pop();
        size.fetch_sub(1);
        
        return task;
    }

    inline int getCapacity() noexcept {
        std::shared_lock<std::shared_mutex> read_lock(rw_mtx);
        return capacity;
    }

    inline int getSize() noexcept {
        return size.load();
    }

private:
    std::queue<std::pair<std::function<void(Args...)>, std::tuple<Args...>>> task_queue;
    std::mutex mtx;
    std::shared_mutex rw_mtx;
    int capacity;
    std::atomic<int> size {0};
};

#endif
