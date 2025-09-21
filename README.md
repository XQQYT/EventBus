# EventBus
[![Windows](https://img.shields.io/badge/platform-Windows-blue?logo=windows)](https://www.microsoft.com/windows) [![Linux](https://img.shields.io/badge/platform-Linux-green?logo=linux)](https://www.linux.org/)
## A lightweight, cross-platform，header-only C++ event bus library that provides an easy-to-use publish-subscribe mechanism.

[中文文档](README_zh.md)
[Tutorial Video](https://www.bilibili.com/video/BV13wWwzPE6a/?vd_source=af81440a64836b1a33af1a82ed3c8609)
---

## Table of Contents
- [Features](#features)
- [Getting Started](#getting-started)
  - [Include the header](#include-the-header)
  - [Initialization](#initialization)
- [Examples](#examples)
  - [1. Using Lambda](#1-using-lambda)
  - [2. Using Member Function](#2-using-member-function)
  - [3. Using Normal Function + Priority Tasks](#3-using-normal-function--priority-tasks)
  - [4. Unsubscribe](#4-unsubscribe)
- [API Reference](#api-reference)
- [Supported Callback Types](#supported-callback-types)
- [Integration Requirements](#integration-requirements)
- [Performance Tips](#performance-tips)
- [License](#license)
- [Contributing](#contributing)
- [Support](#support)

---

## Features

1. **Lightweight**: Header-only, zero dependencies (except for the C++17 standard library)
2. **Easy Integration**: Single-header design, ready to use once included
3. **Thread-Safe**: Built-in thread pool for asynchronous event handling
4. **Type-Safe**: Template-based type-safe event handling
5. **High Performance**: Optimized with modern C++ features
6. **Ease of Use**: Simple API design, quick to get started

---

## Getting Started

### Include the header
```cpp
#include "EventBus/EventBus.hpp"
```

### Initialization
#### It is generally recommended to wrap EventBus as a singleton in real projects, this is only a demonstration
```cpp
EventBus bus;
EventBus::EventBusConfig config{
    EventBus::ThreadModel::DYNAMIC,   // Thread pool model
    EventBus::TaskModel::NORMAL,      // Task model
    2,                                // Minimum threads
    4,                                // Maximum threads
    1024                              // Queue capacity
};
bus.initEventBus(config);
```

---

## Examples

### 1. Using Lambda
```cpp
bus.registerEvent("LambdaTest");
bus.subscribe("LambdaTest", [](int a, int b) {
    std::cout << "LambdaTest: a+b=" << a + b << std::endl;
});
bus.publish("LambdaTest", 77, 88);
```

### 2. Using Member Function
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

### 3. Using Normal Function + Priority Tasks
```cpp
void func(int a, int b) {
    std::cout << "Normal function: a+b=" << a + b << std::endl;
}

bus.registerEvent("NormalFunc");
bus.subscribe("NormalFunc", func);

// Low priority
bus.publish(EventBus::TaskPriority::LOW, "NormalFunc", 1, 2);

// High priority
bus.publish(EventBus::TaskPriority::HIGH, "NormalFunc", 100, 200);
```

### 4. Unsubscribe
```cpp
auto id = bus.subscribe("LambdaTest", [](int x) {
    std::cout << "Received: " << x << std::endl;
});

bus.publish("LambdaTest", 42);
bus.unsubscribe("LambdaTest", id);   // Unsubscribe
```

---

## API Reference

| Method | Description |
|:-------|:------------|
| `registerEvent(eventName)` | Register an event type |
| `subscribe(eventName, callback)` | Subscribe to an event |
| `subscribeSafe(eventName, callback)` | Safe subscription (auto-registers event) |
| `publish(eventName, args...)` | Publish an event (normal task) |
| `publish(priority, eventName, args...)` | Publish an event (with priority) |
| `unsubscribe(eventName, id)` | Unsubscribe from an event |
| `isEventRegistered(eventName)` | Check if an event is registered |

---

## Supported Callback Types
- Lambda expressions  
- Normal function pointers  
- Member functions (via `std::bind` or Lambda wrappers)  
- `std::function`  

---

## Integration Requirements
- **C++ Standard**: C++17 or higher  
- **Compiler**: GCC 7+, Clang 5+, MSVC 2017+  
- **Dependencies**: Only the C++ standard library  

---

## Performance Tips
1. Event names: Use `const` strings or string literals to avoid copies
2. Callbacks: Use reference capture to avoid unnecessary copies
3. Thread safety: Thread-safe by default, but be cautious with shared resources inside callbacks
4. Memory management: Use smart pointers or references for large objects

---

## License
MIT License - See [LICENSE](LICENSE) for details

---

## Contributing
Issues and Pull Requests are welcome!

---

## Support
For questions, please submit:

GitHub Issues: [Report an issue](https://github.com/XQQYT/EventBus/issues)

Email: xqqyt0502@163.com
