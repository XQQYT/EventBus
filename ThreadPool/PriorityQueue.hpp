#include <iostream>
#include <set>
#include <mutex>
#include <functional>
#include <tuple>
#include <semaphore.h>
#include "Queue.h"
template <typename Func, typename Tuple>
struct TaskWrapper {
    Func func;
    Tuple args;
    int priority;
    int insertion_order; 

    TaskWrapper(Func f, Tuple a, int p, int order)
        : func(std::move(f)), args(std::move(a)), priority(p), insertion_order(order) {}

    bool operator<(const TaskWrapper& other) const {
        if (priority == other.priority) {
            return insertion_order > other.insertion_order; 
        }
        return priority < other.priority;
    }
};

template <class... Args>
class ThreadPriorityQueue :public Queue<Args...>{
public:

    explicit ThreadPriorityQueue(int max) noexcept
        : capacity(max), insertionOrder(0) {
        this->size = 0;
        sem_init(&queue_sem,0,0);
    }

    ThreadPriorityQueue() {
        this->capacity = 1024;
        this->size = 0;
        insertionOrder = 0;
    }

    void addTask(int priority, std::function<void(Args...)>&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(mtx);

        if (task_set.size() >= this->capacity) {
            throw std::runtime_error("queue is full");
            return;
        }
        auto arg_tuple=std::make_tuple(std::forward<Args>(args)...);
        task_set.insert(TaskType(std::move(func), std::move(arg_tuple), priority, insertionOrder++ ));
        this->size++;
        sem_post(&queue_sem);
    }

    std::pair<std::function<void(Args...)>, std::tuple<Args...>> getTask() {
        std::lock_guard<std::mutex> lock(mtx);
        if (this->size <= 0) {
            throw std::runtime_error("task queue is empty");
        }
        auto task = *task_set.begin();
        task_set.erase(task_set.begin());
        this->size--;
        return { std::move(task.func), std::move(task.args) };
    }

    inline int getCapacity() noexcept {
        return this->capacity;
    }
    inline int getSize() noexcept {
        return this->size;
    }

    void release() {
        sem_post(&queue_sem);
    }

    void acquire() {
        sem_wait(&queue_sem);
    }

private:
    using TaskType = TaskWrapper<std::function<void(Args...)>, std::tuple<Args...>>;
    std::multiset<TaskType> task_set;
    std::mutex mtx;
    int capacity;
    int size;
    int insertionOrder;
    sem_t queue_sem;
};
