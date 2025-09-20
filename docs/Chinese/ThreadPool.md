### ThreadPool 模块技术文档
#### 概述

ThreadPool 是一个可扩展的线程池，实现任务队列调度和线程动态管理。
核心功能：

- 支持固定或动态线程池

- 支持普通队列或优先级队列

- 可自定义线程池扩缩规则

- 线程安全，多生产者多消费者

#### 枚举与默认值
```c++
enum ThreadPoolType { NORMAL, PRIORITY };
static const int default_thread_min = 8;
static const int default_thread_max = 16;
static const int default_task_max = 1024;
```

- ThreadPoolType：队列类型（普通或优先级）

- 默认线程数和任务队列容量

#### 类定义
```c++
template <class... Args>
class ThreadPool
```

#### 核心数据结构
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

- thread_map：线程 ID → 线程对象

- task_queue：任务队列（普通或优先级）

- manager_thread：动态管理线程池

- thread_size / thread_min / thread_capacity：线程数量管理

- thread_busy_num：忙线程数量

- need_to_close_num：需要关闭的线程数

- shutdown：关闭标志

#### 构造函数
```c++
ThreadPool(int thread_min, int thread_max, int task_queue_max, ThreadPoolType type, bool use_manager, custom_scaling_rule=nullptr);
```

- 初始化线程池

- 创建最小线程数量

- 根据 use_manager 决定是否启用管理线程

#### 公共接口
```c++
void addTask(std::function<void(Args...)> func, Args... args);
void addTask(int priority, std::function<void(Args...)> func, Args... args);
void closeThreadPool();
int getThreadPoolSize() noexcept;
```

- addTask：添加任务，可选择优先级

- closeThreadPool：关闭线程池并释放资源

- getThreadPoolSize：获取当前线程数

#### 内部机制
##### WorkerWorkFunction

- 工作线程循环获取任务执行

- 支持任务阻塞等待

- 动态关闭线程（need_to_close_num）

##### managerWorkFunction

- 定期检查任务队列与线程负载

- 根据自定义或默认扩缩规则调整线程数

- 默认规则：

    - 任务数大于线程数 → 增加线程

    - 忙线程数小于线程数一半 → 删除线程

#### QueueFactory
```c++
static std::unique_ptr<Queue<Args...>> createQueue(ThreadPoolType type, int max_size);
```

- 根据线程池类型选择队列实现：

    - NORMAL → ThreadQueue

    - PRIORITY → ThreadPriorityQueue

#### 队列模块
##### ThreadQueue

- 基于 std::queue

- 普通 FIFO 队列

- 线程安全，使用 mutex + semaphore

##### ThreadPriorityQueue

- 基于 std::multiset<TaskWrapper>

- 支持任务优先级和 FIFO 插入顺序

- 线程安全，使用 mutex + semaphore

#### 使用示例
```c++
ThreadPool<int> pool(4, 8, 128, PRIORITY, true);

pool.addTask(1, [](int a){ std::cout << "Task priority 1: " << a << std::endl; }, 42);
pool.addTask([](int a){ std::cout << "Normal task: " << a << std::endl; }, 100);

pool.closeThreadPool();
```