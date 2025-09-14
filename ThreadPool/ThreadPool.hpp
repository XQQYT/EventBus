#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <iostream>
#include <thread>
#include <functional>
#include "ThreadQueue.hpp"
#include "PriorityQueue.hpp"
#include <chrono>
#include <unordered_map>
#include <vector>

static const int default_thread_min = 8;
static const int default_thread_max = 16;
static const int default_task_max = 1024;

enum ThreadPoolType{
    NORMAL,
    PRIORITY
};

template <typename... Args>

class QueueFactory {

public:

    static std::unique_ptr<Queue<Args...>> createQueue(ThreadPoolType type, int max_size) {
        switch (type) {
            case NORMAL:
                return std::make_unique<ThreadQueue<Args...>>(max_size);
            case PRIORITY:
                return std::make_unique<ThreadPriorityQueue<Args...>>(max_size);
            default:
                throw std::invalid_argument("Unsupported queue type");
        }
    }
};

template <class... Args>
class ThreadPool {
public:
	ThreadPool() {
		shutdown = false;
		thread_busy_num = 0;
		need_to_close_num = 0;
		thread_capacity = default_thread_max;
		thread_size = default_thread_min;
		thread_min = default_thread_min;
		manager_thread = std::thread(&ThreadPool::managerWorkFunction, this);
        task_queue = QueueFactory<Args...>::createQueue(0,default_task_max);
		for (int i = 0; i < default_thread_min; i++) {
			std::thread tmp_thread(&ThreadPool::WorkerWorkFunction, this);
			thread_map.insert({ tmp_thread.get_id(), std::move(tmp_thread) });

		}
	}

	ThreadPool(ThreadPool&&) = delete;
	ThreadPool(const ThreadPool&) = delete;


	~ThreadPool() {
		closeThreadPool();
		manager_thread.join();
		for (auto& thread : thread_map) {
			if(thread.second.joinable())
			{
				std::cout<<"worker thread exit"<<std::endl;
				thread.second.join();
			}
		}
	}


    explicit ThreadPool(const int thread_min_, const int thread_max, const int task_queue_max, const ThreadPoolType type,
		std::function<std::pair<bool, bool>(int task_num, int thread_size, int busy_num)> custom_scaling_rule = nullptr) noexcept
		: thread_min(thread_min_), thread_capacity(thread_max), task_max(task_queue_max) ,scaling_rule(custom_scaling_rule){
		shutdown = false;
		need_to_close_num = 0;
		thread_busy_num = 0;
		thread_capacity = thread_max;
		thread_size = thread_min_;
        task_queue = QueueFactory<Args...>::createQueue(type,task_queue_max);


		manager_thread = std::thread(&ThreadPool::managerWorkFunction, this);
		for (int i = 0; i < thread_min_; i++) {
			std::thread tmp_thread(&ThreadPool::WorkerWorkFunction, this);
			thread_map.insert({ tmp_thread.get_id(), std::move(tmp_thread) });
		}
		
	}

	void addTask(int priority,std::function<void(Args...)> func, Args... args) {
		task_queue->addTask(priority,std::move(func), std::forward<Args>(args)...);
	}
    void addTask(std::function<void(Args...)> func, Args... args) {
        task_queue->addTask(std::move(func), std::forward<Args>(args)...);
    }

	void closeThreadPool() {
		mtx.lock();
		shutdown = true;
		for (int i = 0; i < thread_size; i++)
			task_queue->release();
		mtx.unlock();
		
	}

	inline int getThreadPoolSize() noexcept {
		return thread_size;
	}

private:
	int thread_busy_num;
	int thread_capacity;
	int thread_size;
	int thread_min;
	int task_max;

	std::mutex mtx;
	std::shared_mutex rw_mtx;

    std::unique_ptr<Queue<Args...>> task_queue;

	std::unordered_map<std::thread::id, std::thread> thread_map;

	std::function<std::pair<bool, bool>(int task_num, int thread_size, int busy_num)> scaling_rule;

	std::thread manager_thread;

	std::vector<std::thread::id> need_to_erase;

	bool shutdown;

	int need_to_close_num;

private:
	void managerWorkFunction() {
		while (!shutdown) {
			std::shared_lock<std::shared_mutex> read_lock(rw_mtx);
			int task_num = task_queue->getSize();
			for(auto& id : need_to_erase)
			{
				thread_map.erase(id);
			}
			need_to_erase.clear();
			bool add = false, remove = false;

        	if (scaling_rule) {
            	std::tie(add, remove) = scaling_rule(task_num, thread_size, thread_busy_num);
        	} else {
            	add = (task_num > thread_size && thread_size < thread_capacity);
            	remove = (thread_busy_num * 2 < thread_size && thread_size > thread_min);
        	}

			if (add) {
				std::thread tmp_thread(&ThreadPool::WorkerWorkFunction, this);
				std::unique_lock<std::mutex> write_lock(mtx);
				thread_map.insert({ tmp_thread.get_id(), std::move(tmp_thread) });
				thread_size++;
				write_lock.unlock();
				std::cout << "add a thread" << std::endl;

			}
			else if (remove) {
				std::cout << "kill a thread" << std::endl;
				std::unique_lock<std::mutex> write_lock(mtx);
				need_to_close_num++;
				task_queue->release();
				write_lock.unlock();
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		std::cout << "manager thread exit" << std::endl;
	}

	void WorkerWorkFunction() {
		while (!shutdown) {
			task_queue->acquire();
			if (shutdown)
				break;
			if (need_to_close_num > 0) {
				shutdownThread();
				mtx.lock();
				need_to_close_num--;
				thread_size--;
				mtx.unlock();
				break;
			}
			else {
				auto task = task_queue->getTask();
				auto func = task.first;
                auto args = task.second;

				mtx.lock();
				thread_busy_num++;
				mtx.unlock();
				std::apply(func, args);

				mtx.lock();
				thread_busy_num--;
				mtx.unlock();

			}
		}
		if (shutdown)
		{
			shutdownThread();
		}
	}

	void shutdownThread() {
		std::thread::id id = std::this_thread::get_id();
		std::unique_lock<std::mutex> lock(mtx);
		need_to_erase.push_back(id);
	}
};

#endif
