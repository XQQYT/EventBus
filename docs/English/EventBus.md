## EventBus Technical Documentation

#### Overview

EventBus is a thread-safe event bus system used for event communication and asynchronous task handling across different modules.

#### Core Features:

1. Event registration and management
2. Callback subscription and unsubscription
3. Asynchronous event publishing
4. Support for task prioritization
5. Automatic type deduction for callback parameters

#### Enums and Configuration
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

- **ThreadModel**: Type of thread pool, fixed or dynamic
- **TaskModel**: Task scheduling mode, normal or priority
- **TaskPriority**: Task priority level
- **EventBusConfig**: Initialization parameters, including thread pool configuration and task queue size

#### Class Definition
```c++
class EventBus
```

#### Core Data Structures
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

- **CallbackWrapper**: Encapsulates callback function and subscription ID
- **callbacks_map**: Event name → list of callbacks
- **registered_events**: Set of registered events
- **thread_pool**: Thread pool for asynchronous callback execution
- **next_id**: Generates unique callback IDs

### Public Interfaces
#### Initialization
```c++
void initEventBus(EventBusConfig config);
```

- Initializes the thread pool and task scheduling model
- Throws `runtime_error` for UNDEFINED ThreadModel or TaskModel

#### Event Registration
```c++
void registerEvent(const std::string eventName);
bool isEventRegistered(const std::string eventName) const;
```
- Registers an event or checks if it is registered
- Initializes the callback list upon registration

#### Event Subscription
```c++
template <typename... Args>
callback_id subscribe(const std::string eventName, std::function<void(Args...)> callback);

template <typename Callback>
callback_id subscribe(const std::string eventName, Callback &&callback);
```

- Supports function pointers, lambdas, and member functions
- Automatically deduces callback parameter types
- Returns a unique `callback_id` for unsubscription

#### Safe Subscription (Auto-Register Event)
```c++
template <typename... Args>
callback_id subscribeSafe(const std::string eventName, std::function<void(Args...)> callback);

template <typename Callback>
callback_id subscribeSafe(const std::string eventName, Callback &&callback);
```
- Automatically registers the event if it does not exist

#### Event Publishing
```c++
template <typename... Args>
void publish(const std::string eventName, Args... args);

template <typename... Args>
void publishWithPriority(TaskPriority priority, const std::string eventName, Args... args);
```
- Asynchronously calls all callbacks
- Supports normal and priority tasks
- Uses `std::apply` internally to invoke callbacks

#### Unsubscription
```c++
bool unsubscribe(const std::string eventName, callback_id id);
```
- Removes a subscription by callback ID
- Returns whether the unsubscription was successful

#### Example Usage
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

