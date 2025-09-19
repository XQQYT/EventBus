# EventBus

## A lightweight, header-only C++ EventBus library providing a simple and easy-to-use publish-subscribe mechanism.

## Features

1. **Lightweight**: Header-only, zero dependencies (except C++17 standard library)

2. **Easy Integration**: Single-header design, just include and use

3. **Thread-Safe**: Built-in thread pool support for asynchronous event handling

4. **Type-Safe**: Template-based type-safe event handling

5. **High Performance**: Optimized with modern C++ features

6. **Ease of Use**: Clean API design for quick adoption

## Quick Start

### Include Header File

```cpp
#include "EventBus.hpp"
```

### Basic Usage
```cpp
// Get EventBus instance
auto& bus = EventBus::getInstance();

// Register events
bus.registerEvent("user_login");
bus.registerEvent("data_loaded");

// Subscribe to events
bus.subscribeSafe("user_login", [](const std::string& username, int userId) {
    std::cout << "User logged in: " << username << " (ID: " << userId << ")
";
});

// Publish event
bus.publish("user_login", "john_doe", 12345);

// Asynchronous handling is done automatically via thread pool
```

### API Reference

#### Core Methods
| Method | Description |
|:------ | :------: |
|getInstance() | Get singleton instance |
|registerEvent(eventName) | Register an event type |
|subscribe(eventName, callback) | Subscribe to an event |
|subscribeSafe(eventName, callback) | Safe subscription (auto-registers event) |
|publish(eventName, args...) | Publish an event |
|unsubscribe(eventName, id) | Unsubscribe from an event |
|isEventRegistered(eventName) | Check if an event is registered |

### Event Callback Signatures
#### Supports multiple function types:

- Lambda expressions

- `std::function`

- Function pointers

- Member functions (wrapped with `std::bind` or lambdas)

### Examples

#### Basic Event Handling
```cpp
auto& bus = EventBus::getInstance();

// Subscribe to events
bus.subscribeSafe("app_started", []() {
    std::cout << "Application started!
";
});

bus.subscribeSafe("user_action", [](const std::string& action, int value) {
    std::cout << "User performed: " << action << " with value: " << value << "
";
});

// Publish events
bus.publish("app_started");
bus.publish("user_action", "click", 42);
```

#### Custom Data Structures
```cpp
struct UserData {
    std::string name;
    int age;
    std::vector<std::string> roles;
};

// Subscribe to complex event
bus.subscribeSafe("user_created", [](const UserData& user) {
    std::cout << "New user created: " << user.name 
              << ", Age: " << user.age 
              << ", Roles: " << user.roles.size() << "
";
});

// Publish complex event
UserData newUser{"Alice", 30, {"admin", "user"}};
bus.publish("user_created", newUser);
```

#### Error Handling
```cpp
try {
    bus.publish("unregistered_event", "data"); // throws exception
} catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

### Thread Pool Configuration
#### EventBus includes a built-in thread pool with default configuration:

- Core threads: 2  
- Max threads: 4  
- Task queue capacity: 1024  

```cpp
// Custom configuration can be done in EventBus constructor
EventBus() : thread_pool(std::make_unique<ThreadPool<>>(2, 4, 1024)) {}
```

### Integration Requirements
- **C++ Standard**: C++17 or later  
- **Compilers**: GCC 7+, Clang 5+, MSVC 2017+  
- **Dependencies**: Only requires C++ Standard Library  

### Manual Include
```cpp
// Ensure ThreadPool.hpp is in the same directory or include path
#include "EventBus.hpp"
```

### Performance Tips
1. **Event Names**: Use `const` strings or string literals to avoid copies  
2. **Callbacks**: Capture by reference to reduce unnecessary copies  
3. **Thread Safety**: Thread-safe by default, but manage shared resources carefully in callbacks  
4. **Memory Management**: Use smart pointers or pass by reference for large objects  

## License
MIT License - See [LICENSE](LICENSE) file

## Contribution
Issues and Pull Requests are welcome!

## Support
For questions, please submit:  

GitHub Issues: [Report an issue](https://github.com/XQQYT/EventBus/issues)  

Email: xqqyt0502@163.com
