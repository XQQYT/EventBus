/*
 * EventBus
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <iostream>
#include <thread>
#include <functional>
#include "ThreadQueue.hpp"
#include "PriorityQueue.hpp"
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <vector>

static const unsigned int default_thread_min = 8;
static const unsigned int default_thread_max = 16;
static const unsigned int default_task_max = 1024;

enum ThreadPoolType
{
	NORMAL,
	PRIORITY
};

template <typename... Args>

class QueueFactory
{

public:
	static std::unique_ptr<Queue<Args...>> createQueue(ThreadPoolType type, unsigned int max_size)
	{
		switch (type)
		{
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
class ThreadPool
{

public:
    struct ThreadPoolStatus {
        unsigned int thread_count;           // Current thread count
        unsigned int idle_thread_count;      // Idle thread count  
        unsigned int queue_size;             // Task queue size
        unsigned int total_tasks_processed;  // Total processed tasks
        unsigned int pending_tasks;          // Pending tasks count
        bool is_running;                     // Thread pool running status
    };
public:
	ThreadPool()
	{
		shutdown = false;
		thread_busy_num.store(0);
		need_to_close_num = 0;
		thread_capacity = default_thread_max;
		thread_size.store(default_thread_min);
		thread_min = default_thread_min;
		manager_thread = std::thread(&ThreadPool::managerWorkFunction, this);
		task_queue = QueueFactory<Args...>::createQueue(0, default_task_max);
		for (unsigned int i = 0; i < default_thread_min; i++)
		{
			std::thread tmp_thread(&ThreadPool::WorkerWorkFunction, this);
			thread_map.insert({tmp_thread.get_id(), std::move(tmp_thread)});
		}
	}

	ThreadPool(ThreadPool &&) = delete;
	ThreadPool(const ThreadPool &) = delete;

	~ThreadPool()
	{
		closeThreadPool();
		if (manager_thread.joinable())
		{
			manager_thread.join();
		}
		for (auto &thread : thread_map)
		{
			if (thread.second.joinable())
			{
				thread.second.join();
			}
		}
		std::cout << "ThreadPool have exited" << std::endl;
	}

	explicit ThreadPool(const unsigned int thread_min_, const unsigned int thread_max, const unsigned int task_queue_max, const ThreadPoolType type,
						const bool use_manager, std::function<std::pair<bool, bool>(unsigned int task_num, unsigned int thread_size, unsigned int busy_num)> custom_scaling_rule = nullptr) noexcept
		: thread_min(thread_min_), thread_capacity(thread_max), task_max(task_queue_max), scaling_rule(custom_scaling_rule)
	{
		shutdown = false;
		need_to_close_num = 0;
		thread_busy_num.store(0);
		thread_capacity = thread_max;
		thread_size.store(thread_min_);
		task_queue = QueueFactory<Args...>::createQueue(type, task_queue_max);
		if (use_manager)
		{
			manager_thread = std::thread(&ThreadPool::managerWorkFunction, this);
		}
		for (unsigned int i = 0; i < thread_min_; i++)
		{
			std::thread tmp_thread(&ThreadPool::WorkerWorkFunction, this);
			thread_map.insert({tmp_thread.get_id(), std::move(tmp_thread)});
		}
	}

	void addTask(unsigned int priority, std::function<void(Args...)> func, Args... args)
	{
		task_queue->addTask(priority, std::move(func), std::forward<Args>(args)...);
		cv.notify_one();
	}
	void addTask(std::function<void(Args...)> func, Args... args)
	{
		task_queue->addTask(std::move(func), std::forward<Args>(args)...);
		cv.notify_one();
	}

	void closeThreadPool()
	{
		std::unique_lock<std::mutex> lock(mtx);
		shutdown = true;
		for (unsigned int i = 0; i < thread_size.load(); i++)
			cv.notify_one();
	}

	inline unsigned int getThreadPoolSize() noexcept
	{
		return thread_size.load();
	}

	void updateStatus()
	{
		cur_status.idle_thread_count = thread_size.load() - thread_busy_num;
		cur_status.is_running = shutdown;
		cur_status.pending_tasks = task_queue->getSize();
		cur_status.queue_size = task_queue->getCapacity();
		cur_status.thread_count = thread_size.load();
		cur_status.total_tasks_processed = processed_num;
	}

	const ThreadPoolStatus& getStatus()
	{
		updateStatus();
		return cur_status;
	}

	void resetStatistics()
	{
		processed_num.store(0);
	}


private:
	std::atomic<unsigned int> thread_busy_num;
	std::atomic<unsigned int> processed_num {};
	unsigned int thread_capacity;
	std::atomic<unsigned int> thread_size;
	unsigned int thread_min;
	unsigned int task_max;

	std::mutex mtx;

	std::unique_ptr<Queue<Args...>> task_queue;

	std::unordered_map<std::thread::id, std::thread> thread_map;

	std::function<std::pair<bool, bool>(unsigned int task_num, unsigned int thread_size, unsigned int busy_num)> scaling_rule;

	std::thread manager_thread;

	std::vector<std::thread::id> need_to_erase;

	bool shutdown;

	std::atomic<unsigned int> need_to_close_num;

	std::condition_variable cv;

	ThreadPoolStatus cur_status;
private:
	void managerWorkFunction()
	{
		while (!shutdown)
		{
			unsigned int task_num = task_queue->getSize();
			bool add = false, remove = false;

			if (scaling_rule)
				std::tie(add, remove) = scaling_rule(task_num, thread_size, thread_busy_num.load());
			else
			{
				add = (task_num > thread_size && thread_size < thread_capacity);
				remove = (thread_busy_num.load() * 2 < thread_size && thread_size > thread_min);
			}

			if (add)
			{
				std::thread new_thread(&ThreadPool::WorkerWorkFunction, this);
				{
					std::lock_guard<std::mutex> lk(mtx);
					thread_map.emplace(new_thread.get_id(), std::move(new_thread));
				}
				thread_size.fetch_add(1,std::memory_order_relaxed);
				cv.notify_one();
			}
			if (remove)
			{
				need_to_close_num.fetch_add(1,std::memory_order_relaxed);
				cv.notify_one();
			}

			std::vector<std::thread> threads_to_join;
			{
				std::lock_guard<std::mutex> lk(mtx);
				for (auto &id : need_to_erase)
				{
					auto it = thread_map.find(id);
					if (it != thread_map.end())
					{
						threads_to_join.emplace_back(std::move(it->second));
						thread_map.erase(it);
					}
				}
				need_to_erase.clear();
			}
			for (auto &t : threads_to_join)
			{
				if (t.joinable())
					t.join();
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		cv.notify_all();
	}

	void WorkerWorkFunction()
	{
		while (true)
		{
			std::function<void(Args...)> func;
			std::tuple<std::decay_t<Args>...> args;

			{
				std::unique_lock<std::mutex> lock(mtx);
				cv.wait(lock, [this]
						{ return shutdown || need_to_close_num.load() > 0 || task_queue->getSize() > 0; });
			}
			if (shutdown && task_queue->getSize() == 0)
			{
				std::unique_lock<std::mutex> lock(mtx);
				need_to_erase.push_back(std::this_thread::get_id());
				break;
			}

			if (need_to_close_num.load() > 0)
			{
				need_to_close_num.fetch_sub(1,std::memory_order_relaxed);
				thread_size.fetch_sub(1,std::memory_order_relaxed);
				std::unique_lock<std::mutex> lock(mtx);
				need_to_erase.push_back(std::this_thread::get_id());
				break;
			}

			try
			{
				auto task = task_queue->getTask();
				func = std::move(task.first);
				args = std::move(task.second);
				thread_busy_num.fetch_add(1,std::memory_order_relaxed);
				std::apply(func, args);
				thread_busy_num.fetch_sub(1,std::memory_order_relaxed);
				processed_num.fetch_add(1,std::memory_order_relaxed);
			}
			catch(const std::exception& e)
			{
				// std::cerr << e.what() << '\n';
			}
		}
	}

	void shutdownThread()
	{
		std::thread::id id = std::this_thread::get_id();
		std::unique_lock<std::mutex> lock(mtx);

		auto it = thread_map.find(id);
		if (it != thread_map.end() && it->second.joinable())
		{
			need_to_erase.push_back(id);
		}
	}
};

#endif
