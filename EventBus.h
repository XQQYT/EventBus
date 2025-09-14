/*
 * EventBus
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

// function_traits 定义
template<typename T>
struct function_traits;

// 普通函数
template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)> {
    using signature = Ret(Args...);
};

// 函数指针
template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : function_traits<Ret(Args...)> {};

// std::function
template<typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> : function_traits<Ret(Args...)> {};

// 成员函数指针
template<typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret(ClassType::*)(Args...)> : function_traits<Ret(Args...)> {};

// const 成员函数指针
template<typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret(ClassType::*)(Args...) const> : function_traits<Ret(Args...)> {};

// 函数对象（包括lambda）
template<typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())> {};

// 特化处理 std::bind
template<typename Callable, typename... Args>
struct function_traits<std::_Bind<Callable(Args...)>> 
    : function_traits<Callable> {};

using callback_id = size_t;

class NetworkController;
class WebrtcController;
class ConfigController;
class EventBus {
public:
    EventBus(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus& operator=(EventBus&&) = delete;
    enum class EventType;
    
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    void registerEvent(EventType eventName) {
        auto [it, inserted] = registered_events.emplace(std::move(eventName));
        if (inserted) {
            callbacks_map.try_emplace(*it).first->second.reserve(3);
        }
    }

    // 显式模板参数的 subscribe
    template<typename... Args>
    callback_id subscribe(const EventType eventName, std::function<void(Args...)> callback) {
        if (!isEventRegistered(eventName)) {
            throw std::runtime_error("Event not registered: " + std::to_string(static_cast<int>(eventName)));
        }
        callback_id id = ++next_id;
        callbacks_map[eventName].emplace_back(CallbackWrapper{id, callback});
        return id;
    }

    // 自动推导 Callback 类型的 subscribe
    template<typename Callback>
    callback_id subscribe(EventType eventName, Callback&& callback) {
        using signature = typename function_traits<std::decay_t<Callback>>::signature;
        return subscribe(eventName, std::function<signature>(std::forward<Callback>(callback)));
    }

    // 安全订阅版本（自动注册事件）
    template<typename... Args>
    callback_id subscribeSafe(const EventType eventName, std::function<void(Args...)> callback) {
        if (!isEventRegistered(eventName)) {
            registerEvent(eventName);
        }
        return subscribe(eventName, callback);
    }
    
    // 自动推导的安全订阅版本
    template<typename Callback>
    callback_id subscribeSafe(EventType eventName, Callback&& callback) {
        using signature = typename function_traits<std::decay_t<Callback>>::signature;
        return subscribeSafe(eventName, std::function<signature>(std::forward<Callback>(callback)));
    }

    template<typename... Args>
    void publish(EventType eventName, Args... args) {
        if (!isEventRegistered(eventName)) {
            throw std::runtime_error("Event not registered: " + std::to_string(static_cast<int>(eventName)));
        }
        
        auto args_tuple = std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...);

        for (auto& wrapper : callbacks_map[eventName]) {
            thread_pool->addTask([wrapper, args_tuple]() {
                try {
                    if (auto cb = std::any_cast<std::function<void(Args...)>>(&wrapper.callback)) {
                        std::apply(*cb, *args_tuple);
                    } 
                    else if (auto cb = std::any_cast<std::function<void()>>(&wrapper.callback)) {
                        (*cb)();
                    }
                } catch (...) {
                    std::cerr << "Callback execution failed for event: " << wrapper.id << "\n";
                }
            });
        }
    }

    bool isEventRegistered(const EventType eventName) const {
        return registered_events.find(eventName) != registered_events.end();
    }

    bool unsubscribe(const EventType eventName, callback_id id) {
        if (!isEventRegistered(eventName)) return false;
        auto& callbacks = callbacks_map[eventName];
        for (auto it = callbacks.begin(); it != callbacks.end(); ) {
            if (it->id == id) {
                it = callbacks.erase(it);
                return true;
            } else {
                ++it;
            }
        }
        return false;
    }
    void initModuleSubscribe();
    void registerAllEvents();
private:
    EventBus();
    ~EventBus();
    struct CallbackWrapper {
        callback_id id;
        std::any callback;
    };
    std::unordered_map<EventType, std::vector<CallbackWrapper>> callbacks_map;
    std::unordered_set<EventType> registered_events;
    std::atomic<callback_id> next_id{0};
    std::unique_ptr<ThreadPool<>> thread_pool;

    std::unique_ptr<NetworkController> network_controller;
    std::unique_ptr<WebrtcController> webrtc_controller;
    std::unique_ptr<ConfigController> config_controller;
};

enum class EventBus::EventType {
    // UI
    UI_ConnectWidgetInitDone,

    // Network
    Network_GetTargetStatus,
    Network_ConnectToSignalServerResult,
    Network_SignalRegisterResult,
    Network_TargetStatus,
    Network_ConnectToTarget,
    Network_HasConnectRequest,
    Network_SendConnectRequestResult,
    Network_RecvConnectRequestResult,
    Network_SendLogout,
    Network_SendToUserServer,
    Network_ConnectToUserServerResult,
    Network_ReceiveDeviceCode,
    Network_ReConnectToSignalServer,
    Network_LoginResult,
    Network_UserRegisterResult,
    Network_UserAvatarUpdate,
    Network_UpdateDeviceList,
    Network_UpdateDeviceCommentResult,
    Network_DeleteDeviceInConfig,
    Network_DeleteDeviceResult,
    Network_UploadAvatarResult,
    Network_UpdateUsernameResult,
    Network_UpdateUserPasswordResult,
    Network_RegisterDeviceResult,
    Network_GetDeviceListResult,
    Network_GetLastestVersion,
    Network_LastestVersionResult,
    Network_GetVersionPackage,
    Network_VersionPackageProgress,

    // WebRTC
    WebRTC_InitWebrtcDone,
    WebRTC_RemoteReady,
    WebRTC_CreateSdpOffer,
    WebRTC_SetRemoteSdpOfferDone,
    WebRTC_CreateSdpAnswer,
    WebRTC_RecvSdpOffer,
    WebRTC_RecvSdpAnswer,
    WebRTC_SendIceCandidate,
    WebRTC_RecvIceCandidate,
    WebRTC_SendIceGatherDone,
    WebRTC_RecvIceGatherDone,
    WebRTC_ConnectionStatus,
    WebRTC_SyncFileInfo,//发送方
    WebRTC_SyncAddFile,//接收方
    WebRTC_SetFileHolder,
    WebRTC_ReceiveFileProgess,
    WebRTC_PeerconnectionStatus,

    // Render
    Render_SetRenderInstance,

    // Input
    MouseEvent_HasEvent,
    KeyboardEvent_HasEvent,
    Clipboard_WriteIntoClipboard,

    // Control
    Control_CloseControl,
    Control_RecvCloseControl,

    // Config
    Config_UpdateModuleConfig,
    Config_UpdateModuleConfigDone,
    Config_WriteIntoFile,
    Config_ModuleConfigUpdated,
    Config_CopyFile,
    Config_RenameFile,
    Config_UpdateDeviceComment
};


#endif