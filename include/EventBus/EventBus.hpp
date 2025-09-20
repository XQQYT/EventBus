/*
 * XqqytDesktop
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 */

#ifndef _EVENTBUS_H
#define _EVENTBUS_H

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <any>
#include <atomic>
#include <memory>
#include <type_traits>
#include <thread>

#include "ThreadPool/ThreadPool.hpp"

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
{
};

// std::function
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)>
{
};

// Member function pointer
template <typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret (ClassType::*)(Args...)> : function_traits<Ret(Args...)>
{
};

// const member function pointer
template <typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret (ClassType::*)(Args...) const> : function_traits<Ret(Args...)>
{
};

// Function objects (including lambda)
template <typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())>
{
};

// Specialization for std::bind
template <typename Callable, typename... Args>
struct function_traits<std::_Bind<Callable(Args...)>>
    : function_traits<Callable>
{
};

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
    };

public:
    /**
     * @brief Construct a new EventBus object
     */
    EventBus()
    {
        init_status = false;
    };
    /**
     * @brief Destroy the EventBus object
     */
    ~EventBus(){};
    EventBus(const EventBus &) = delete;
    EventBus(EventBus &&) = delete;
    EventBus &operator=(const EventBus &) = delete;

    /**
     * @brief Initialize the EventBus with configuration
     * @param config EventBusConfig object
     * @throw runtime_error if configuration is invalid
     */
    void initEventBus(EventBusConfig config)
    {
        if (config.thread_model == ThreadModel::UNDEFINED)
        {
            throw std::runtime_error("Invalid ThreadModel : " + std::to_string(static_cast<int>(config.thread_model)));
        }
        this->config = config;
        if (config.thread_model == ThreadModel::DYNAMIC)
        {
            if (config.task_model == TaskModel::NORMAL)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min, config.thread_max, config.task_max, ThreadPoolType::NORMAL, true);
            }
            else if (config.task_model == TaskModel::PRIORITY)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min, config.thread_max, config.task_max, ThreadPoolType::PRIORITY, true);
            }
            else
            {
                throw std::runtime_error("Invalid TaskModel : " + std::to_string(static_cast<int>(config.task_model)));
            }
        }
        else if (config.thread_model == ThreadModel::FIXED)
        {
            if (config.task_model == TaskModel::NORMAL)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min, config.thread_min, config.task_max, ThreadPoolType::NORMAL, false);
            }
            else if (config.task_model == TaskModel::PRIORITY)
            {
                thread_pool = std::make_unique<ThreadPool<>>(config.thread_min, config.thread_min, config.task_max, ThreadPoolType::PRIORITY, false);
            }
            else
            {
                throw std::runtime_error("Invalid TaskModel : " + std::to_string(static_cast<int>(config.task_model)));
            }
        }
        else
        {
            throw std::runtime_error("Invalid ThreadModel : " + std::to_string(static_cast<int>(config.thread_model)));
        }
        init_status = true;
        task_model = config.task_model;
    }

    /**
     * @brief Register an event with a given name
     * @param eventName Name of the event
     */
    void registerEvent(const std::string eventName)
    {
        if (!init_status)
        {
            throw std::runtime_error("EventBus has not been initialized");
        }
        auto [it, inserted] = registered_events.emplace(std::move(eventName));
        if (inserted)
        {
            callbacks_map.try_emplace(*it).first->second.reserve(3);
        }
    }

    /**
     * @brief Subscribe to an event with explicit std::function signature
     * @tparam Args Event argument types
     * @param eventName Event name
     * @param callback Callback function
     * @return callback_id Unique subscription ID
     */
    template <typename... Args>
    callback_id subscribe(const std::string eventName, std::function<void(Args...)> callback)
    {
        if (!isEventRegistered(eventName))
        {
            throw std::runtime_error("Event not registered: " + eventName);
        }
        callback_id id = ++next_id;
        callbacks_map[eventName].emplace_back(CallbackWrapper{id, callback});
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
    callback_id subscribe(const std::string eventName, Callback &&callback)
    {
        if (!init_status)
        {
            throw std::runtime_error("EventBus has not been initialized");
        }
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
    callback_id subscribeSafe(const std::string eventName, std::function<void(Args...)> callback)
    {
        if (!isEventRegistered(eventName))
        {
            registerEvent(eventName);
        }
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
    callback_id subscribeSafe(const std::string eventName, Callback &&callback)
    {
        if (!init_status)
        {
            throw std::runtime_error("EventBus has not been initialized");
        }
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
    void publish(const std::string eventName, Args... args)
    {
        if (!init_status)
        {
            throw std::runtime_error("EventBus has not been initialized");
        }
        if (!isEventRegistered(eventName))
        {
            throw std::runtime_error("Event not registered: " + eventName);
        }

        auto args_tuple = std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...);

        for (auto &wrapper : callbacks_map[eventName])
        {
            if (task_model == TaskModel::NORMAL)
            {
                thread_pool->addTask([wrapper, args_tuple](){
                    try {
                        if (auto cb = std::any_cast<std::function<void(Args...)>>(&wrapper.callback)) {
                            std::apply(*cb, *args_tuple);
                        } 
                        else if (auto cb = std::any_cast<std::function<void()>>(&wrapper.callback)) {
                            (*cb)();
                        }
                    } catch (...) {
                        std::cerr << "Callback execution failed for event: " << wrapper.id << "\n";
                } });
            }else if(task_model == TaskModel::PRIORITY){
                thread_pool->addTask(static_cast<int>(TaskPriority::MIDDLE),[wrapper, args_tuple](){
                    try {
                        if (auto cb = std::any_cast<std::function<void(Args...)>>(&wrapper.callback)) {
                            std::apply(*cb, *args_tuple);
                        } 
                        else if (auto cb = std::any_cast<std::function<void()>>(&wrapper.callback)) {
                            (*cb)();
                        }
                    } catch (...) {
                        std::cerr << "Callback execution failed for event: " << wrapper.id << "\n";
                } });
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
    void publish(TaskPriority priority, const std::string eventName, Args... args)
    {
        if (!init_status)
        {
            throw std::runtime_error("EventBus has not been initialized");
        }
        if (!isEventRegistered(eventName))
        {
            throw std::runtime_error("Event not registered: " + eventName);
        }

        auto args_tuple = std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...);

        for (auto &wrapper : callbacks_map[eventName])
        {
            if (task_model == TaskModel::NORMAL)
            {
                thread_pool->addTask([wrapper, args_tuple](){
                    try {
                        if (auto cb = std::any_cast<std::function<void(Args...)>>(&wrapper.callback)) {
                            std::apply(*cb, *args_tuple);
                        } 
                        else if (auto cb = std::any_cast<std::function<void()>>(&wrapper.callback)) {
                            (*cb)();
                        }
                    } catch (...) {
                        std::cerr << "Callback execution failed for event: " << wrapper.id << "\n";
                } });
            }else if(task_model == TaskModel::PRIORITY){
                thread_pool->addTask(static_cast<int>(priority),[wrapper, args_tuple](){
                    try {
                        if (auto cb = std::any_cast<std::function<void(Args...)>>(&wrapper.callback)) {
                            std::apply(*cb, *args_tuple);
                        } 
                        else if (auto cb = std::any_cast<std::function<void()>>(&wrapper.callback)) {
                            (*cb)();
                        }
                    } catch (...) {
                        std::cerr << "Callback execution failed for event: " << wrapper.id << "\n";
                } });
            }
        }
    }

    /**
     * @brief Check if an event is registered
     * @param eventName Event name
     * @return true If registered
     * @return false Otherwise
     */
    bool isEventRegistered(const std::string eventName) const
    {
        return registered_events.find(eventName) != registered_events.end();
    }

    /**
     * @brief Unsubscribe a callback by ID
     * @param eventName Event name
     * @param id Subscription ID
     * @return true If unsubscribed successfully
     * @return false If not found
     */
    bool unsubscribe(const std::string eventName, callback_id id)
    {
        if (!init_status)
        {
            throw std::runtime_error("EventBus has not been initialized");
        }
        if (!isEventRegistered(eventName))
            return false;
        auto &callbacks = callbacks_map[eventName];
        for (auto it = callbacks.begin(); it != callbacks.end();)
        {
            if (it->id == id)
            {
                it = callbacks.erase(it);
                return true;
            }
            else
            {
                ++it;
            }
        }
        return false;
    }

private:
    struct CallbackWrapper
    {
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
};

#endif