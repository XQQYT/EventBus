/*
 * EventBus
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 */

#ifndef _EVENTBUS_H
#define _EVENTBUS_H

#include <algorithm>
#include <any>
#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <optional>

#include "ThreadPool/ThreadPool.hpp"

class EventBusException : public std::exception
{
public:
    explicit EventBusException(std::string msg) : message(std::move(msg)) {}
    const char* what() const noexcept override { return message.c_str(); }

protected:
    std::string message;
};

class EventBusNotInitializedException : public EventBusException
{
public:
    using EventBusException::EventBusException;
};

class EventBusConfigurationException : public EventBusException
{
public:
    using EventBusException::EventBusException;
};

class EventNotRegisteredException : public EventBusException
{
public:
    using EventBusException::EventBusException;
};

class TaskModelMismatchException : public EventBusException
{
public:
    using EventBusException::EventBusException;
};

// function_traits definitions
template <typename T>
struct function_traits;

// Normal function
template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
    using signature = Ret(Args...);
};

// Function pointer
template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : function_traits<Ret(Args...)>
{};

// std::function
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)>
{};

// Member function pointer
template <typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret (ClassType::*)(Args...)> : function_traits<Ret(Args...)>
{};

// const member function pointer
template <typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret (ClassType::*)(Args...) const> : function_traits<Ret(Args...)>
{};

// Function objects (including lambda)
template <typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())>
{};

#ifdef _MSC_VER
template <typename Ret, typename ClassType, typename... Args, typename... BoundArgs>
struct function_traits<std::_Binder<std::_Unforced, Ret (ClassType::*)(Args...), BoundArgs...>>
{
    using signature = Ret(Args...);
};
#else
template <typename Callable, typename... Args>
struct function_traits<std::_Bind<Callable(Args...)>> : function_traits<Callable>
{};
#endif

using callback_id = size_t;

class EventBus
{
public:
    enum class ThreadModel : int
    {
        FIXED = 0,
        DYNAMIC = 1,
        UNDEFINED = -1
    };

    enum class TaskModel
    {
        NORMAL,
        PRIORITY
    };

    enum class TaskPriority
    {
        HIGH,
        MIDDLE,
        LOW
    };

    struct EventBusConfig
    {
        ThreadModel thread_model = ThreadModel::UNDEFINED;
        TaskModel task_model;
        unsigned int thread_min;
        unsigned int thread_max;
        unsigned int task_max;

        EventBusConfig() = default;

        EventBusConfig(ThreadModel tm,
                       TaskModel tsk_model,
                       unsigned int t_min,
                       unsigned int t_max,
                       unsigned int tsk_max)
            : thread_model(tm),
              task_model(tsk_model),
              thread_min(t_min),
              thread_max(t_max),
              task_max(tsk_max)
        {
            validateConfig(*this);
        }

        static void validateConfig(const  EventBusConfig& config)
        {
            if (config.thread_min <= 0)
            {
                throw EventBusConfigurationException(
                    "Invalid EventBus config: thread_min must be > 0, got " +
                    std::to_string(config.thread_min));
            }

            if (config.thread_max <= 0)
            {
                throw EventBusConfigurationException(
                    "Invalid EventBus config: thread_max must be > 0, got " +
                    std::to_string(config.thread_max));
            }
            if (config.thread_min > config.thread_max)
            {
                throw EventBusConfigurationException("Invalid EventBus config: thread_min (" +
                                                     std::to_string(config.thread_min) +
                                                     ") cannot be greater than thread_max (" +
                                                     std::to_string(config.thread_max) + ")");
            }

            if (config.thread_model == ThreadModel::UNDEFINED)
            {
                throw EventBusConfigurationException(
                    "Invalid ThreadModel : " + std::to_string(static_cast<int>(config.thread_model)));
            }
        }
    };

    struct EventSystemStatus {
        size_t registered_events_count;      // Registered events count
        size_t total_subscriptions;          // Total subscriptions
        size_t events_triggered_count;       // Events triggered count
        size_t events_failed_count;          // Events failed count
        size_t active_subscriptions;         // Active subscriptions count
        std::unordered_map<std::string, size_t> event_subscription_count; // Subscriptions per event
    };

    struct EventBusStatus {
        ThreadPool<>::ThreadPoolStatus thread_pool_status; // Thread pool status
        EventSystemStatus event_system_status; // Event system status
        bool is_initialized;                 // Initialization status
        EventBus::ThreadModel thread_model;  // Thread model
        EventBus::TaskModel task_model;      // Task model
        unsigned int max_threads;            // Max threads limit
        unsigned int max_tasks;              // Max tasks limit
    };

public:
    /**
     * @brief Construct a new EventBus object
     */
    EventBus() = default;
    /**
     * @brief Destroy the EventBus object
     */
    virtual ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    /**
     * @brief Initialize the EventBus with configuration
     * @param config EventBusConfig object
     * @throw runtime_error if configuration is invalid
     */
    void initEventBus(const EventBusConfig& config)
    {
        EventBusConfig::validateConfig(config);
        this->config = config;
        if (config.thread_model == ThreadModel::DYNAMIC)
        {
            if (config.task_model == TaskModel::NORMAL)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min,
                                                             config.thread_max,
                                                             config.task_max,
                                                             ThreadPoolType::NORMAL,
                                                             true);
            }
            else if (config.task_model == TaskModel::PRIORITY)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min,
                                                             config.thread_max,
                                                             config.task_max,
                                                             ThreadPoolType::PRIORITY,
                                                             true);
            }
            else
            {
                throw EventBusConfigurationException(
                    "Invalid TaskModel : " + std::to_string(static_cast<int>(config.task_model)));
            }
        }
        else if (config.thread_model == ThreadModel::FIXED)
        {
            if (config.task_model == TaskModel::NORMAL)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min,
                                                             config.thread_min,
                                                             config.task_max,
                                                             ThreadPoolType::NORMAL,
                                                             false);
            }
            else if (config.task_model == TaskModel::PRIORITY)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min,
                                                             config.thread_min,
                                                             config.task_max,
                                                             ThreadPoolType::PRIORITY,
                                                             false);
            }
            else
            {
                throw EventBusConfigurationException(
                    "Invalid TaskModel : " + std::to_string(static_cast<int>(config.task_model)));
            }
        }

        init_status = true;
        task_model = config.task_model;
    }

    /**
     * @brief Register an event with a given name
     * @param eventName Name of the event
     */
    void registerEvent(const std::string& eventName)
    {
        ensureInitialized();
        auto [it, inserted] = callbacks_map.try_emplace(eventName);
        if (inserted) { it->second.reserve(3); }
    }

    /**
     * @brief Subscribe to an event with explicit std::function signature
     * @tparam Args Event argument types
     * @param eventName Event name
     * @param callback Callback function
     * @return callback_id Unique subscription ID
     */
    template <typename... Args>
    callback_id subscribe(const std::string& eventName, std::function<void(Args...)> callback)
    {
        auto it = callbacks_map.find(eventName);
        if (it == callbacks_map.end())
        {
            throw EventNotRegisteredException("Event not registered: " + eventName);
        }
        event_statistics[eventName].subscription_count++;
        callback_id id = ++next_id;
        it->second.emplace_back(CallbackWrapper {id, std::move(callback)});
        return id;
    }

    /**
     * @brief Subscribe to an event with automatic callback type deduction
     * @tparam Callback Callback type
     * @param eventName Event name
     * @param callback Callback function
     * @return callback_id Unique subscription ID
     */
    template <typename Callback>
    callback_id subscribe(const std::string& eventName, Callback&& callback)
    {
        ensureInitialized();
        using signature = typename function_traits<std::decay_t<Callback>>::signature;
        return subscribe(eventName, std::function<signature>(std::forward<Callback>(callback)));
    }

    /**
     * @brief Subscribe to an event safely (auto-register if not exists)
     * @tparam Args Event argument types
     * @param eventName Event name
     * @param callback Callback function
     * @return callback_id Unique subscription ID
     */
    template <typename... Args>
    callback_id subscribeSafe(const std::string& eventName, std::function<void(Args...)> callback)
    {
        registerEvent(eventName);
        return subscribe(eventName, callback);
    }

    /**
     * @brief Safe subscribe with automatic type deduction
     * @tparam Callback Callback type
     * @param eventName Event name
     * @param callback Callback function
     * @return callback_id Unique subscription ID
     */
    template <typename Callback>
    callback_id subscribeSafe(const std::string eventName, Callback&& callback)
    {
        registerEvent(eventName);
        using signature = typename function_traits<std::decay_t<Callback>>::signature;
        return subscribeSafe(eventName, std::function<signature>(std::forward<Callback>(callback)));
    }

    /**
     * @brief Publish an event (normal task)
     * @tparam Args Event argument types
     * @param eventName Event name
     * @param args Event arguments
     */
    template <typename... Args>
    void publish(const std::string& eventName, Args&&... args)
    {
        ensureInitialized();
        auto it = callbacks_map.find(eventName);
        if (it == callbacks_map.end())
        {
            throw EventNotRegisteredException("Event not registered: " + eventName);
        }

        events_triggered_count.fetch_add(1, std::memory_order_relaxed);
        auto& event_stats = event_statistics[eventName];
        event_stats.triggered_count.fetch_add(1, std::memory_order_relaxed);

        if constexpr (sizeof...(Args) == 0)
        {
            for (auto& wrapper : it->second)
            {
                if (task_model == TaskModel::NORMAL)
                {
                    thread_pool->addTask(
                        [this,&wrapper,&eventName]
                        {
                            try
                            {
                                if (auto cb =
                                        std::any_cast<std::function<void()>>(&wrapper.callback))
                                {
                                    (*cb)();
                                }
                            }
                            catch (...)
                            {
                                events_failed_count.fetch_add(1, std::memory_order_relaxed);
                                event_statistics[eventName].failed_count.fetch_add(1, std::memory_order_relaxed);
                                std::cerr << "Callback execution failed for event: " << wrapper.id
                                          << "\n";
                            }
                        });
                }
                else if (task_model == TaskModel::PRIORITY)
                {
                    throw TaskModelMismatchException(
                        "Cannot use normal-based publishing in PRIORITY task model");
                }
            }
        }
        else
        {
            auto args_tuple =
                std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...);

            for (auto& wrapper : it->second)
            {
                if (task_model == TaskModel::NORMAL)
                {
                    thread_pool->addTask(
                        [this,&wrapper, args_tuple]()
                        {
                            try
                            {
                                if (auto cb = std::any_cast<std::function<void(Args...)>>(
                                        &wrapper.callback))
                                {
                                    std::apply(*cb, *args_tuple);
                                }
                                else if (auto cb = std::any_cast<std::function<void()>>(
                                             &wrapper.callback))
                                {
                                    (*cb)();
                                }
                            }
                            catch (const std::exception& e)
                            {
                                std::cerr << "Callback execution failed for event: " << wrapper.id
                                          << ", error: " << e.what() << "\n";
                            }
                            catch (...)
                            {
                                std::cerr << "Unknown error in callback execution for event: "
                                          << wrapper.id << "\n";
                            }
                        });
                }
                else if (task_model == TaskModel::PRIORITY)
                {
                    throw TaskModelMismatchException(
                        "Cannot use normal-based publishing in PRIORITY task model");
                }
            }
        }
    }

    /**
     * @brief Publish an event with priority
     * @tparam Args Event argument types
     * @param priority TaskPriority
     * @param eventName Event name
     * @param args Event arguments
     */
    template <typename... Args>
    void publishWithPriority(TaskPriority priority, const std::string& eventName, Args&&... args)
    {
        ensureInitialized();
        auto it = callbacks_map.find(eventName);
        if (it == callbacks_map.end())
        {
            throw EventNotRegisteredException("Event not registered: " + eventName);
        }

        if constexpr (sizeof...(Args) == 0)
        {
            for (auto& wrapper : it->second)
            {
                if (task_model == TaskModel::NORMAL)
                {
                    throw TaskModelMismatchException(
                        "Cannot use priority-based publishing in NORMAL task model");
                }
                if (task_model == TaskModel::PRIORITY)
                {
                    thread_pool->addTask(
                        static_cast<int>(priority),
                        [this,&wrapper]()
                        {
                            try
                            {
                                if (auto cb =
                                        std::any_cast<std::function<void()>>(&wrapper.callback))
                                {
                                    (*cb)();
                                }
                            }
                            catch (...)
                            {
                                std::cerr << "Callback execution failed for event: " << wrapper.id
                                          << "\n";
                            }
                        });
                }
            }
        }
        else
        {
            auto args_tuple =
                std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...);

            for (auto& wrapper : it->second)
            {
                if (task_model == TaskModel::NORMAL)
                {
                    throw TaskModelMismatchException(
                        "Cannot use priority-based publishing in NORMAL task model");
                }
                if (task_model == TaskModel::PRIORITY)
                {
                    thread_pool->addTask(
                        static_cast<int>(priority),
                        [this,&wrapper, args_tuple]()
                        {
                            try
                            {
                                if (auto cb = std::any_cast<std::function<void(Args...)>>(
                                        &wrapper.callback))
                                {
                                    std::apply(*cb, *args_tuple);
                                }
                                else if (auto cb = std::any_cast<std::function<void()>>(
                                             &wrapper.callback))
                                {
                                    (*cb)();
                                }
                            }
                            catch (...)
                            {
                                std::cerr << "Callback execution failed for event: " << wrapper.id
                                          << "\n";
                            }
                        });
                }
            }
        }
    }

    /**
     * @brief Check if an event is registered
     * @param eventName Event name
     * @return true If registered
     * @return false Otherwise
     */
    bool isEventRegistered(const std::string& eventName) const
    {
        return callbacks_map.count(eventName) > 0;
    }

    /**
     * @brief Unsubscribe a callback by ID
     * @param eventName Event name
     * @param id Subscription ID
     * @return true If unsubscribed successfully
     * @return false If not found
     */
    bool unsubscribe(const std::string& eventName, callback_id id)
    {
        ensureInitialized();

        auto iter = callbacks_map.find(eventName);
        if (iter == callbacks_map.end()) { return false; }

        auto& callbacks = iter->second;
        auto it = std::find_if(callbacks.begin(),
                               callbacks.end(),
                               [id](const CallbackWrapper& wrapper) { return wrapper.id == id; });
        if (it != callbacks.end())
        {
            callbacks.erase(it);
            auto stats_it = event_statistics.find(eventName);
            if (stats_it != event_statistics.end() && stats_it->second.subscription_count > 0)
            {
                stats_it->second.subscription_count--;
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieve the complete state of the EventBus
     * @return EventBusStatus contains the complete status of the thread pool and event system
     */
    EventBusStatus getStatus() const
    {
        EventBusStatus status;
        
        // Basic status
        status.is_initialized = init_status;
        if (init_status)
        {
            status.thread_model = config.thread_model;
            status.task_model = config.task_model;
            status.max_threads = config.thread_max;
            status.max_tasks = config.task_max;
            
            // Threadpool status
            if (thread_pool)
            {
                auto pool_status = thread_pool->getStatus();
                status.thread_pool_status.thread_count = pool_status.thread_count;
                status.thread_pool_status.idle_thread_count = pool_status.idle_thread_count;
                status.thread_pool_status.queue_size = pool_status.queue_size;
                status.thread_pool_status.total_tasks_processed = pool_status.total_tasks_processed;
                status.thread_pool_status.pending_tasks = pool_status.pending_tasks;
                status.thread_pool_status.is_running = pool_status.is_running;
            }
            
            // Event system status
            status.event_system_status.registered_events_count = callbacks_map.size();
            status.event_system_status.events_triggered_count = events_triggered_count.load();
            status.event_system_status.events_failed_count = events_failed_count.load();
            
            // Calculate subscription statistics
            size_t total_subscriptions = 0;
            for (const auto& [event_name, callbacks] : callbacks_map)
            {
                size_t event_subs = callbacks.size();
                total_subscriptions += event_subs;
                status.event_system_status.event_subscription_count[event_name] = event_subs;
            }
            status.event_system_status.total_subscriptions = total_subscriptions;
            status.event_system_status.active_subscriptions = total_subscriptions;
        }
        
        return status;
    }

    /**
     * @brief Get simplified status information (better performance)
     * @return A simplified struct containing key states
     */
    struct SimplifiedStatus {
        bool is_initialized;
        unsigned int thread_count;
        unsigned int queue_size;
        size_t registered_events;
        size_t total_subscriptions;
        size_t events_triggered;
        size_t events_failed;
    };
    
    SimplifiedStatus getSimplifiedStatus() const
    {
        SimplifiedStatus status;
        status.is_initialized = init_status;
        
        if (init_status && thread_pool)
        {
            auto pool_status = thread_pool->getStatus();
            status.thread_count = pool_status.thread_count;
            status.queue_size = pool_status.queue_size;
            
            status.registered_events = callbacks_map.size();
            status.events_triggered = events_triggered_count.load();
            status.events_failed = events_failed_count.load();
            
            size_t total_subs = 0;
            for (const auto& [_, callbacks] : callbacks_map)
            {
                total_subs += callbacks.size();
            }
            status.total_subscriptions = total_subs;
        }
        
        return status;
    }

    /**
     * @brief Get statistical information for a specific event
     * @param eventName Event name
     * @return Event statistical information. If the event does not exist, an empty optional is returned.
     */
    struct EventStatistics {
        size_t subscription_count;
        size_t triggered_count;
        size_t failed_count;
        double success_rate;
    };
    
    std::optional<EventStatistics> getEventStatistics(const std::string& eventName) const
    {
        auto event_it = event_statistics.find(eventName);
        if (event_it == event_statistics.end())
        {
            return std::nullopt;
        }
        
        const auto& stats = event_it->second;
        EventStatistics result;
        result.subscription_count = stats.subscription_count;
        result.triggered_count = stats.triggered_count.load();
        result.failed_count = stats.failed_count.load();
        
        if (result.triggered_count > 0)
        {
            result.success_rate = (1.0 - static_cast<double>(result.failed_count) / 
                                 static_cast<double>(result.triggered_count)) * 100.0;
        }
        else
        {
            result.success_rate = 100.0;
        }
        
        return result;
    }

    /**
     * @brief 重置统计计数器
     * @param reset_events 是否重置事件触发统计
     * @param reset_threadpool 是否重置线程池统计
     */
    void resetStatistics(bool reset_events = true, bool reset_threadpool = false)
    {
        if (reset_events)
        {
            events_triggered_count.store(0);
            events_failed_count.store(0);
            for (auto& [_, stats] : event_statistics)
            {
                stats.triggered_count.store(0);
                stats.failed_count.store(0);
            }
        }
        
        if (reset_threadpool && thread_pool)
        {
            thread_pool->resetStatistics();
        }
    }

private:
    /**
     * @brief Ensure EventBus is initialized before operation
     * @throws EventBusNotInitializedException if EventBus is not initialized
     */
    void ensureInitialized() const
    {
        if (!init_status)
        {
            throw EventBusNotInitializedException("EventBus has not been initialized");
        }
    }

    struct CallbackWrapper
    {
        callback_id id;
        std::any callback;
    };
    std::unordered_map<std::string, std::vector<CallbackWrapper>> callbacks_map;
    std::atomic<callback_id> next_id {0};
    std::unique_ptr<ThreadPool<>> thread_pool;
    EventBusConfig config;
    bool init_status {};
    TaskModel task_model;

        struct EventStats {
        std::atomic<size_t> triggered_count{0};
        std::atomic<size_t> failed_count{0};
        size_t subscription_count{0};
    };
    
    std::atomic<size_t> events_triggered_count{0};
    std::atomic<size_t> events_failed_count{0};
    std::unordered_map<std::string, EventStats> event_statistics;
};

#endif
