#ifndef THREADQUEUE_H
#define THREADQUEUE_H

#include <iostream>
#include <queue>
#include <semaphore.h>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include "Queue.h"

static const int default_capacity=1024;

template <class... Args>
class ThreadQueue:public Queue<Args...>{
public:

	explicit ThreadQueue(int max) noexcept
		:capacity(max) 
	{
		this->size = 0;
        sem_init(&queue_sem,0,0);
	}

	ThreadQueue() 
	{
		this->capacity = default_capacity;
		this->size = 0;
	};

	
	void addTask(std::function<void(Args...)>&& func, Args&&... args)
	{
		std::shared_lock<std::shared_mutex> read_lock(rw_mtx);
		if (task_queue.size() >= this->capacity) {
			throw std::runtime_error("queue is full");
			return;
		}
		mtx.lock();
        auto arg_tuple=std::make_tuple(std::forward<Args>(args)...);
        task_queue.push({std::move(func), std::move(arg_tuple)});
		this->size++;
		mtx.unlock();
        sem_post(&queue_sem);
	}

	std::pair<std::function<void(Args...)>, std::tuple<Args...>> getTask()
	{
		if (this->size <= 0)
		{
			throw std::runtime_error("task queue is empty");
		}
		mtx.lock();
		std::pair<std::function<void(Args...)>, std::tuple<Args...>> task = std::move(task_queue.front());
		task_queue.pop();
		this->size--;
		mtx.unlock();
		
		return task;
	}

	inline int getCapacity() noexcept
	{
		std::shared_lock<std::shared_mutex> read_lock(rw_mtx);
		int capacity_tmp = this->capacity;
		return capacity_tmp;
	}
	inline int getSize() noexcept
	{
		std::shared_lock<std::shared_mutex> read_lock(rw_mtx);
		int size_tmp = this->size;
		return size_tmp;
	}
	void release()
	{
        sem_post(&queue_sem);
	}

	void acquire()
	{
        sem_wait(&queue_sem);
	}

private:
	std::queue<std::pair<std::function<void(Args...)>, std::tuple<Args...>>> task_queue;

	std::mutex mtx;
	std::shared_mutex rw_mtx;
	int capacity;
	int size;
    sem_t queue_sem;
};



#endif


