### ThreadPool Module Technical Documentation

#### Overview

ThreadPool is an extensible thread pool implementation that manages task queues and dynamic thread scaling.

Core features:
- Supports fixed or dynamic thread pools
- Supports normal or priority task queues
- Customizable thread scaling rules
- Thread-safe, multi-producer, multi-consumer

#### Enums and Default Values
```c++
enum ThreadPoolType { NORMAL, PRIORITY };
static const int default_thread_min = 8;
static const int default_thread_max = 16;
static const int default_task_max = 1024;
```
- **ThreadPoolType**: Queue type (normal or priority)
- Default thread counts and task queue capacity

#### Class Definition
```c++
template <class... Args>
class ThreadPool
```

#### Core Data Structures
```c++
std::unordered_map<std::thread::id, std::thread> thread_map;
std::unique_ptr<Queue<Args...>> task_queue;
std::thread manager_thread;
std::mutex mtx;
std::shared_mutex rw_mtx;

int thread_size;
int thread_min;
int thread_capacity;
int thread_busy_num;
int need_to_close_num;
bool shutdown;
```
- **thread_map**: Thread ID → thread object
- **task_queue**: Task queue (normal or priority)
- **manager_thread**: Dynamically manages thread pool
- **thread_size / thread_min / thread_capacity**: Thread number management
- **thread_busy_num**: Number of busy threads
- **need_to_close_num**: Threads to close
- **shutdown**: Shutdown flag

#### Constructor
```c++
ThreadPool(int thread_min, int thread_max, int task_queue_max, ThreadPoolType type, bool use_manager, custom_scaling_rule=nullptr);
```
- Initializes the thread pool
- Creates minimum number of threads
- Uses manager thread if `use_manager` is true

#### Public Interfaces
```c++
void addTask(std::function<void(Args...)> func, Args... args);
void addTask(int priority, std::function<void(Args...)> func, Args... args);
void closeThreadPool();
int getThreadPoolSize() noexcept;
```
- **addTask**: Adds a task, optionally with priority
- **closeThreadPool**: Closes the pool and releases resources
- **getThreadPoolSize**: Returns current thread count

#### Internal Mechanisms
##### WorkerWorkFunction
- Worker thread loops to fetch and execute tasks
- Supports blocking wait for tasks
- Can dynamically close threads (`need_to_close_num`)

##### managerWorkFunction
- Periodically checks task queue and thread workload
- Adjusts thread count according to custom or default scaling rules
- Default rules:
    - Task count > thread count → add thread
    - Busy threads < half of thread count → remove thread

#### QueueFactory
```c++
static std::unique_ptr<Queue<Args...>> createQueue(ThreadPoolType type, int max_size);
```
- Selects queue implementation based on thread pool type:
    - **NORMAL** → ThreadQueue
    - **PRIORITY** → ThreadPriorityQueue

#### Queue Modules
##### ThreadQueue
- Based on `std::queue`
- Normal FIFO queue
- Thread-safe using `mutex + semaphore`

##### ThreadPriorityQueue
- Based on `std::multiset<TaskWrapper>`
- Supports task priority and FIFO insertion order
- Thread-safe using `mutex + semaphore`

#### Example Usage
```c++
ThreadPool<int> pool(4, 8, 128, PRIORITY, true);

pool.addTask(1, [](int a){ std::cout << "Task priority 1: " << a << std::endl; }, 42);
pool.addTask([](int a){ std::cout << "Normal task: " << a << std::endl; }, 100);

pool.closeThreadPool();
```

