/*
 * EventBus
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 *
 * Optimized: 2026-06-26
 * Changes:
 *   1. shutdown: bool → std::atomic<bool> — 消除 release/store 之间的 TOCTOU 竞态
 *   2. closeThreadPool(): cv.notify_all() 替代按线程数 notify_one — 避免与 manager 扩缩容竞态
 *   3. Manager 用 cv.wait_for 替代 sleep_for — shutdown 时立即唤醒，析构不再阻塞 1s
 *   4. Manager 创建线程前检查 shutdown — 防止关闭后继续创建 worker
 *   5. updateStatus().is_running 反转修复 (was: shutdown, now: !shutdown)
 *   6. Worker shutdown 时排空最后一个任务 — 避免残留
 *   7. 删除死代码 shutdownThread()
 *   8. QueueFactory::createQueue(0, ...) → 显式传 ThreadPoolType::NORMAL
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
		shutdown.store(false);
		thread_busy_num.store(0);
		need_to_close_num.store(0);
		thread_capacity = default_thread_max;
		thread_size.store(default_thread_min);
		thread_min = default_thread_min;
		task_queue = QueueFactory<Args...>::createQueue(ThreadPoolType::NORMAL, default_task_max);
		for (unsigned int i = 0; i < default_thread_min; i++)
		{
			std::thread tmp_thread(&ThreadPool::WorkerWorkFunction, this);
			thread_map.insert({tmp_thread.get_id(), std::move(tmp_thread)});
		}
		manager_thread = std::thread(&ThreadPool::managerWorkFunction, this);
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
		// 兜底：join 所有尚未被 manager reap 的 worker 线程
		std::vector<std::thread> remaining;
		{
			std::lock_guard<std::mutex> lk(mtx);
			for (auto& kv : thread_map) {
				remaining.emplace_back(std::move(kv.second));
			}
			thread_map.clear();
		}
		for (auto &t : remaining)
		{
			if (t.joinable()) t.join();
		}
	}

	explicit ThreadPool(const unsigned int thread_min_, const unsigned int thread_max,
	                    const unsigned int task_queue_max, const ThreadPoolType type,
						const bool use_manager,
	                    std::function<std::pair<bool, bool>(unsigned int task_num,
	                        unsigned int thread_size, unsigned int busy_num)> custom_scaling_rule = nullptr) noexcept
		: thread_min(thread_min_), thread_capacity(thread_max), task_max(task_queue_max),
		  scaling_rule(custom_scaling_rule)
	{
		shutdown.store(false);
		need_to_close_num.store(0);
		thread_busy_num.store(0);
		thread_capacity = thread_max;
		thread_size.store(thread_min_);
		task_queue = QueueFactory<Args...>::createQueue(type, task_queue_max);
		for (unsigned int i = 0; i < thread_min_; i++)
		{
			std::thread tmp_thread(&ThreadPool::WorkerWorkFunction, this);
			thread_map.insert({tmp_thread.get_id(), std::move(tmp_thread)});
		}
		if (use_manager)
		{
			manager_thread = std::thread(&ThreadPool::managerWorkFunction, this);
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

	// 修复: notify_all 替代按线程数 notify_one，消除与 manager 扩缩容的竞态
	void closeThreadPool()
	{
		shutdown.store(true, std::memory_order_release);
		cv.notify_all();
		manager_cv.notify_one();   // 立即唤醒 manager，避免析构阻塞 1 秒
	}

	inline unsigned int getThreadPoolSize() noexcept
	{
		return thread_size.load();
	}

	void updateStatus()
	{
		cur_status.idle_thread_count = thread_size.load() - thread_busy_num.load();
		cur_status.is_running = !shutdown.load(std::memory_order_acquire);  // 修复: 原为 shutdown (语义反转)
		cur_status.pending_tasks = task_queue->getSize();
		cur_status.queue_size = task_queue->getCapacity();
		cur_status.thread_count = thread_size.load();
		cur_status.total_tasks_processed = processed_num.load();
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

	std::atomic<bool> shutdown{false};                  // 修复: bool → atomic<bool>

	std::atomic<unsigned int> need_to_close_num;

	std::condition_variable cv;
	std::condition_variable manager_cv;                  // 新增: 独立 CV 用于唤醒 manager

	ThreadPoolStatus cur_status;

private:
	// 修复: sleep_for(1s) → wait_for(1s)，shutdown 时立即唤醒
	void managerWorkFunction()
	{
		while (!shutdown.load(std::memory_order_acquire))
		{
			unsigned int task_num = task_queue->getSize();
			unsigned int current_size = thread_size.load(std::memory_order_acquire);
			unsigned int busy = thread_busy_num.load(std::memory_order_acquire);
			bool add = false, remove = false;

			if (scaling_rule)
				std::tie(add, remove) = scaling_rule(task_num, current_size, busy);
			else
			{
				add = (task_num > current_size && current_size < thread_capacity);
				remove = (busy * 2 < current_size && current_size > thread_min);
			}

			// 修复: 创建线程前检查 shutdown，防止关闭后继续扩容
			if (add && !shutdown.load(std::memory_order_acquire))
			{
				std::thread new_thread(&ThreadPool::WorkerWorkFunction, this);
				{
					std::lock_guard<std::mutex> lk(mtx);
					thread_map.emplace(new_thread.get_id(), std::move(new_thread));
				}
				thread_size.fetch_add(1, std::memory_order_release);
				cv.notify_one();
			}
			if (remove)
			{
				need_to_close_num.fetch_add(1, std::memory_order_release);
				cv.notify_one();
			}

			// 收割已退出的 worker 线程
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
				if (t.joinable()) t.join();
			}

			// 修复: wait_for 替代 sleep_for，可被 closeThreadPool 立即唤醒
			{
				std::unique_lock<std::mutex> lock(mtx);
				manager_cv.wait_for(lock, std::chrono::seconds(1),
					[this] { return shutdown.load(std::memory_order_acquire); });
			}
		}

		cv.notify_all();
	}

	void WorkerWorkFunction()
	{
		while (true)
		{
			{
				std::unique_lock<std::mutex> lock(mtx);
				cv.wait(lock, [this] {
					return shutdown.load(std::memory_order_acquire) ||
					       need_to_close_num.load(std::memory_order_acquire) > 0 ||
					       task_queue->getSize() > 0;
				});
			}

			// 修复: shutdown 时尝试排空最后一个任务，避免残留
			if (shutdown.load(std::memory_order_acquire))
			{
				try {
					auto task = task_queue->getTask();
					thread_busy_num.fetch_add(1, std::memory_order_relaxed);
					std::apply(task.first, task.second);
					thread_busy_num.fetch_sub(1, std::memory_order_relaxed);
					processed_num.fetch_add(1, std::memory_order_relaxed);
				} catch (...) {
					// 队列已空，安全退出
				}
				{
					std::lock_guard<std::mutex> lk(mtx);
					need_to_erase.push_back(std::this_thread::get_id());
				}
				break;
			}

			// 缩容：当前线程被标记为退休
			if (need_to_close_num.load(std::memory_order_acquire) > 0)
			{
				need_to_close_num.fetch_sub(1, std::memory_order_relaxed);
				thread_size.fetch_sub(1, std::memory_order_relaxed);
				{
					std::lock_guard<std::mutex> lk(mtx);
					need_to_erase.push_back(std::this_thread::get_id());
				}
				break;
			}

			// 正常执行任务
			try
			{
				auto task = task_queue->getTask();
				thread_busy_num.fetch_add(1, std::memory_order_relaxed);
				std::apply(task.first, task.second);
				thread_busy_num.fetch_sub(1, std::memory_order_relaxed);
				processed_num.fetch_add(1, std::memory_order_relaxed);
			}
			catch (const std::exception&)
			{
				// 伪唤醒 或 getTask 竞态失败 → 重新进入 wait 循环
			}
		}
	}

	// 已删除: shutdownThread() — 死代码，从未被调用
};

#endif
