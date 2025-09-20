## EventBus 技术文档
#### 概述

EventBus 是一个线程安全的事件总线系统，用于在不同模块之间进行事件通信和异步任务处理。

#### 核心功能：

1. 事件注册与管理

2. 回调订阅与取消订阅

3. 异步事件发布

4. 支持任务优先级

5. 自动类型推导回调参数

#### 枚举与配置
```c++
enum class ThreadModel { FIXED, DYNAMIC, UNDEFINED };
enum class TaskModel { NORMAL, PRIORITY };
enum class TaskPriority { HIGH, MIDDLE, LOW };

struct EventBusConfig {
    ThreadModel thread_model = ThreadModel::UNDEFINED;
    TaskModel task_model;
    unsigned int thread_min;
    unsigned int thread_max;
    unsigned int task_max;
};
```

- ThreadModel：线程池类型，固定或动态

- TaskModel：任务调度方式，普通或优先级

- TaskPriority：任务优先级

- EventBusConfig：初始化参数，包括线程池配置和任务队列大小

#### 类定义
```c++
class EventBus
```

#### 核心数据结构
```c++
struct CallbackWrapper {
    callback_id id;
    std::any callback;
};

std::unordered_map<std::string, std::vector<CallbackWrapper>> callbacks_map;
std::unordered_set<std::string> registered_events;
std::atomic<callback_id> next_id{0};
std::unique_ptr<ThreadPool<>> thread_pool;
EventBusConfig config;
bool init_status;
TaskModel task_model;
```

- CallbackWrapper：封装回调函数和订阅 ID

- callbacks_map：事件名 → 回调列表

- registered_events：已注册事件集合

- thread_pool：用于异步执行事件回调

- next_id：生成唯一回调 ID

### 公共接口
#### 初始化
```c++
void initEventBus(EventBusConfig config);
```

- 初始化线程池和任务调度模型

- 异常：
    - UNDEFINED 的 ThreadModel 或 TaskModel 抛出 runtime_error

#### 事件注册
```c++
void registerEvent(const std::string eventName);
bool isEventRegistered(const std::string eventName) const;
```
- 注册事件或查询是否已注册

- 注册时初始化回调列表

#### 订阅事件
```c++
template <typename... Args>
callback_id subscribe(const std::string eventName, std::function<void(Args...)> callback);

template <typename Callback>
callback_id subscribe(const std::string eventName, Callback &&callback);
```

- 支持函数指针、lambda、成员函数

- 自动类型推导回调参数

- 返回唯一 callback_id 用于取消订阅

#### 安全订阅（自动注册事件）
```c++
template <typename... Args>
callback_id subscribeSafe(const std::string eventName, std::function<void(Args...)> callback);

template <typename Callback>
callback_id subscribeSafe(const std::string eventName, Callback &&callback);
```

- 如果事件未注册，会自动注册

#### 发布事件
```c++
template <typename... Args>
void publish(const std::string eventName, Args... args);

template <typename... Args>
void publish(TaskPriority priority, const std::string eventName, Args... args);
```

- 异步调用所有回调

- 支持普通任务和优先级任务

- 内部使用 std::apply 调用回调

#### 取消订阅
```c++
bool unsubscribe(const std::string eventName, callback_id id);
```

- 通过回调 ID 删除订阅回调

- 返回是否取消成功

#### 使用示例
```c++
EventBus bus;
EventBus::EventBusConfig config;
config.thread_model = EventBus::ThreadModel::DYNAMIC;
config.task_model = EventBus::TaskModel::PRIORITY;
config.thread_min = 4;
config.thread_max = 8;
config.task_max = 128;

bus.initEventBus(config);
bus.registerEvent("OnDataReceived");

auto id = bus.subscribe("OnDataReceived", [](int a, std::string b){
    std::cout << "Data: " << a << ", " << b << std::endl;
});

bus.publish("OnDataReceived", 42, std::string("Hello"));
bus.unsubscribe("OnDataReceived", id);
```