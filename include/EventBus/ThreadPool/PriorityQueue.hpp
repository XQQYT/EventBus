/*
 * EventBus
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <functional>
#include <tuple>
#include "Queue.h"

template <typename Func, typename Tuple>
struct TaskWrapper {
    Func func;
    Tuple args;
    unsigned int priority;
    unsigned int insertion_order;

    TaskWrapper(Func f, Tuple a, unsigned int p, unsigned int order)
        : func(std::move(f)), args(std::move(a)), priority(p), insertion_order(order) {}

    bool operator<(const TaskWrapper& other) const {
        if (priority == other.priority) {
            return insertion_order > other.insertion_order; // 时间顺序：数字小的优先
        }
        return priority > other.priority; // 优先级：数字大的优先
    }
};

template <class... Args>
class ThreadPriorityQueue : public Queue<Args...> {
public:
    explicit ThreadPriorityQueue(int max) noexcept
        : capacity(max), insertionOrder(0) {
        task_queue.reserve(max);
    }

    ThreadPriorityQueue() {
        this->capacity = 1024;
        insertionOrder = 0;
        task_queue.reserve(this->capacity);
    }

    void addTask(unsigned int priority, std::function<void(Args...)>&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(mtx);

        if (task_queue.size() >= this->capacity) {
            throw std::runtime_error("queue is full");
        }
        
        auto arg_tuple = std::make_tuple(std::forward<Args>(args)...);
        
        task_queue.push_back(TaskType(
            std::move(func), 
            std::move(arg_tuple), 
            priority, 
            insertionOrder++
        ));
        
        std::push_heap(task_queue.begin(), task_queue.end());
    }

    std::pair<std::function<void(Args...)>, std::tuple<Args...>> getTask() {
        std::lock_guard<std::mutex> lock(mtx);
        if (task_queue.size() <= 0) {
            throw std::runtime_error("task queue is empty");
        }
        
        std::pop_heap(task_queue.begin(), task_queue.end());
        auto task = std::move(task_queue.back());
        task_queue.pop_back();
        
        return { std::move(task.func), std::move(task.args) };
    }

    inline unsigned int getCapacity() noexcept {
        return this->capacity;
    }
    
    inline unsigned int getSize() noexcept {
        return task_queue.size();
    }

private:
    using TaskType = TaskWrapper<std::function<void(Args...)>, std::tuple<Args...>>;
    std::vector<TaskType> task_queue;
    std::mutex mtx;
    unsigned int capacity;
    unsigned int insertionOrder;
};