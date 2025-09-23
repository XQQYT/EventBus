# EventBus
[![Windows](https://img.shields.io/badge/platform-Windows-blue?logo=windows)](https://www.microsoft.com/windows) [![Linux](https://img.shields.io/badge/platform-Linux-green?logo=linux)](https://www.linux.org/)
## 一个轻量级、跨平台，仅头文件的 C++事件总线库，提供简单易用的事件发布-订阅机制。

[教程](https://www.bilibili.com/video/BV13wWwzPE6a/?vd_source=af81440a64836b1a33af1a82ed3c8609)

---

# 目录

- [EventBus](#eventbus)
- [特性](#特性)
- [快速开始](#快速开始)
  - [包含头文件](#包含头文件)
  - [初始化](#初始化)
- [示例](#示例)
  - [1. 使用 Lambda](#1-使用-lambda)
  - [2. 使用成员函数](#2-使用成员函数)
  - [3. 使用普通函数 + 优先级任务](#3-使用普通函数--优先级任务)
  - [4. 取消订阅](#4-取消订阅)
- [API 参考](#api-参考)
- [事件回调支持](#事件回调支持)
- [集成要求](#集成要求)
- [性能建议](#性能建议)
- [许可证](#许可证)
- [贡献](#贡献)
  - [支持](#支持)

---

## 特性

1. **轻量级**: 仅头文件，零依赖（除C++17标准库外）
2. **易集成**: 单头文件设计，包含即可使用
3. **线程安全**: 内置线程池支持，异步事件处理
4. **类型安全**: 基于模板的类型安全事件处理
5. **高性能**: 使用现代C++特性优化性能
6. **易用性**: 简洁的API设计，快速上手

---

## 快速开始

### 包含头文件
```cpp
#include "EventBus/EventBus.hpp"
```

### 初始化
#### 通常建议将 EventBus 封装为单例使用，此处仅作示例
```cpp
EventBus bus;
EventBus::EventBusConfig config{
    EventBus::ThreadModel::DYNAMIC,   // 线程池模式
    EventBus::TaskModel::NORMAL,      // 任务模式
    2,                                // 最小线程数
    4,                                // 最大线程数
    1024                              // 队列容量
};
bus.initEventBus(config);
```

---

## 示例

### 1. 使用 Lambda
```cpp
bus.registerEvent("LambdaTest");
bus.subscribe("LambdaTest", [](int a, int b) {
    std::cout << "LambdaTest: a+b=" << a + b << std::endl;
});
bus.publish("LambdaTest", 77, 88);
```

### 2. 使用成员函数
```cpp
class Handler {
public:
    void memberFunc(int a, int b) {
        std::cout << "Member function: a+b=" << a + b << std::endl;
    }
};

Handler obj;
bus.registerEvent("MemberFunc");
bus.subscribe("MemberFunc", std::bind(&Handler::memberFunc, obj, std::placeholders::_1, std::placeholders::_2));
bus.publish("MemberFunc", 10, 20);
```

### 3. 使用普通函数 + 优先级任务
```cpp
void func(int a, int b) {
    std::cout << "Normal function: a+b=" << a + b << std::endl;
}

bus.registerEvent("NormalFunc");
bus.subscribe("NormalFunc", func);

// 低优先级
bus.publishWithPriority(EventBus::TaskPriority::LOW, "NormalFunc", 1, 2);

// 高优先级
bus.publishWithPriority(EventBus::TaskPriority::HIGH, "NormalFunc", 100, 200);
```

### 4. 取消订阅
```cpp
auto id = bus.subscribe("LambdaTest", [](int x) {
    std::cout << "Received: " << x << std::endl;
});

bus.publish("LambdaTest", 42);
bus.unsubscribe("LambdaTest", id);   // 取消订阅
```

---

## API 参考

| 方法 | 描述 |
|:-----|:------|
| `registerEvent(eventName)` | 注册事件类型 |
| `subscribe(eventName, callback)` | 订阅事件 |
| `subscribeSafe(eventName, callback)` | 安全订阅（自动注册） |
| `publish(eventName, args...)` | 发布事件（普通任务） |
| `publishWithPriority(priority, eventName, args...)` | 发布事件（带优先级） |
| `unsubscribe(eventName, id)` | 取消订阅 |
| `isEventRegistered(eventName)` | 检查事件是否注册 |

---

## 事件回调支持
- Lambda 表达式  
- 普通函数指针  
- 成员函数（`std::bind` 或 Lambda 包装）  
- `std::function`  

---

## 集成要求
- **C++ 标准**: C++17 或更高  
- **编译器**: GCC 7+, Clang 5+, MSVC 2017+  
- **依赖**: 仅需要 C++ 标准库  

---

### 性能建议
1. 事件名称: 使用const字符串或字符串字面量避免拷贝

2. 回调函数: 使用引用捕获避免不必要的拷贝

3. 线程安全: 默认线程安全，但注意回调函数中的资源共享

4. 内存管理: 大型对象使用智能指针或引用传递

---

## 许可证
MIT License - 详见 [LICENSE](LICENSE) 文件

---

## 贡献
欢迎提交Issue和Pull Request！

#### 支持
如有问题请提交：

GitHub Issues: [Report an issue](https://github.com/XQQYT/EventBus/issues)

邮箱: xqqyt0502@163.com