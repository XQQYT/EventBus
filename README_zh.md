# EventBus

## 一个轻量级、仅头文件的C++事件总线库，提供简单易用的事件发布-订阅机制。

## 特性

1. 轻量级: 仅头文件，零依赖（除C++17标准库外）

2. 易集成: 单头文件设计，包含即可使用

3. 线程安全: 内置线程池支持，异步事件处理

4. 类型安全: 基于模板的类型安全事件处理

5. 高性能: 使用现代C++特性优化性能

6. 易用性: 简洁的API设计，快速上手

## 快速开始

### 包含头文件

```cpp
#include "EventBus.hpp"
```

### 基本用法
```cpp
// 获取EventBus实例
auto& bus = EventBus::getInstance();

// 注册事件
bus.registerEvent("user_login");
bus.registerEvent("data_loaded");

// 订阅事件
bus.subscribeSafe("user_login", [](const std::string& username, int userId) {
    std::cout << "User logged in: " << username << " (ID: " << userId << ")\n";
});

// 发布事件
bus.publish("user_login", "john_doe", 12345);

// 异步处理，自动使用线程池
```

### API 参考

#### 核心方法
| 方法 | 描述 |
|:----- | :------: |
|getInstance() |  获取单例实例  |
|registerEvent(eventName) | 注册事件类型 |
|subscribe(eventName, callback) | 订阅事件 |
|subscribeSafe(eventName, callback)	| 安全订阅（自动注册）|
|publish(eventName, args...) | 发布事件 |
|unsubscribe(eventName, id) | 取消订阅 |
|isEventRegistered(eventName) | 检查事件是否注册 |

### 事件回调签名
#### 支持多种回调函数类型：

- Lambda表达式

- std::function

- 函数指针

- 成员函数（需使用std::bind或lambda包装）

### 示例

#### 基本事件处理
``` cpp
auto& bus = EventBus::getInstance();

// 订阅事件
bus.subscribeSafe("app_started", []() {
    std::cout << "Application started!\n";
});

bus.subscribeSafe("user_action", [](const std::string& action, int value) {
    std::cout << "User performed: " << action << " with value: " << value << "\n";
});

// 发布事件
bus.publish("app_started");
bus.publish("user_action", "click", 42);
```

#### 自定义数据结构
```cpp
struct UserData {
    std::string name;
    int age;
    std::vector<std::string> roles;
};

// 订阅复杂事件
bus.subscribeSafe("user_created", [](const UserData& user) {
    std::cout << "New user created: " << user.name 
              << ", Age: " << user.age 
              << ", Roles: " << user.roles.size() << "\n";
});

// 发布复杂事件
UserData newUser{"Alice", 30, {"admin", "user"}};
bus.publish("user_created", newUser);
```

#### 错误处理
```cpp
try {
    bus.publish("unregistered_event", "data"); // 抛出异常
} catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

### 线程池配置
#### EventBus内置线程池，默认配置：

核心线程数: 2

最大线程数: 4

任务队列容量: 1024

```cpp
// 在EventBus构造函数中可自定义配置
EventBus() : thread_pool(std::make_unique<ThreadPool<>>(2, 4, 1024)) {}
```

### 集成要求
C++标准: C++17 或更高

编译器: 支持C++17的编译器（GCC 7+, Clang 5+, MSVC 2017+）

依赖: 仅需要C++标准库

### 手动包含
```cpp
// 确保ThreadPool.hpp在相同目录或包含路径中
#include "EventBus.hpp"
```

### 性能建议
1. 事件名称: 使用const字符串或字符串字面量避免拷贝

2. 回调函数: 使用引用捕获避免不必要的拷贝

3. 线程安全: 默认线程安全，但注意回调函数中的资源共享

4. 内存管理: 大型对象使用智能指针或引用传递

许可证
MIT License - 详见[LICENSE](LICENSE)文件

贡献
欢迎提交Issue和Pull Request！

支持
如有问题请提交：

GitHub Issues: [Report an issue](https://github.com/XQQYT/EventBus/issues)

邮箱: xqqyt0502@163.com