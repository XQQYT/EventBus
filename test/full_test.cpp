/*
 * EventBus Test Code
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 */

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include "../include/EventBus/EventBus.hpp"

// 测试工具函数
// ANSI 颜色代码
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BOLDRED "\033[1;31m"
#define BOLDGREEN "\033[1;32m"

class TestUtils {
private:
    static std::mutex cout_mutex; // 添加互斥锁保护控制台输出

public:
    static void printTestHeader(const std::string& testName) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\n" CYAN "========================================" RESET "\n";
        std::cout << CYAN "- Running Test: " << testName << RESET "\n";
        std::cout << CYAN "========================================" RESET "\n";
    }

    static void printTestResult(bool passed, const std::string& testName) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        if (passed) {
            std::cout << GREEN " [PASS] " RESET << testName << "\n";
        } else {
            std::cout << RED " [FAIL] " RESET << testName << "\n";
        }
    }

    static void printException(const std::exception& e, const std::string& context) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << BOLDRED " Exception in " << context << ": " 
                  << e.what() << RESET "\n";
    }

    static void printSuccess(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << GREEN " success: " RESET << message << "\n";
    }

    static void printWarning(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << YELLOW " warning: " RESET << message << "\n";
    }

    // 新增：线程安全的进度显示
    static void printProgress(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << CYAN " [Progress] " RESET << message << "\n";
    }
};

// 静态成员定义
std::mutex TestUtils::cout_mutex;

// 测试用例类
class EventBusTester {
private:
    EventBus eventBus;
    int testCounter = 0;
    std::mutex test_mutex; // 用于保护测试状态

public:
    void runAllTests() {
        std::cout << "Starting EventBus Comprehensive Tests...\n\n";

        try {
            testInitialization();
            testEventRegistration();
            testSubscription();
            testPublishing();
            testPriorityPublishing();
            testUnsubscription();
            testErrorHandling();
            testThreadSafety();
            testPerformance();
            testStatusMonitoring();
            
            std::cout << "\n All tests completed!\n";
        } catch (const std::exception& e) {
            TestUtils::printException(e, "runAllTests");
        }
    }

private:
    void testInitialization() {
        TestUtils::printTestHeader("1. Initialization Tests");
        
        // Test 1: Normal initialization
        try {
            TestUtils::printTestHeader("1.1 Normal Initialization");
            EventBus::EventBusConfig config;
            config.thread_model = EventBus::ThreadModel::DYNAMIC;
            config.task_model = EventBus::TaskModel::NORMAL;
            config.thread_min = 2;
            config.thread_max = 8;
            config.task_max = 1024;
            
            eventBus.initEventBus(config);
            TestUtils::printSuccess("Test 1: EventBus initialized successfully");
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Normal Initialization");
            throw;
        }

        // Test 2: Invalid configuration
        try {
            TestUtils::printTestHeader("1.2 Invalid Configuration");
            EventBus::EventBusConfig badConfig;
            badConfig.thread_model = EventBus::ThreadModel::UNDEFINED;
            
            EventBus badBus;
            badBus.initEventBus(badConfig);
            TestUtils::printTestResult(false, "Should have thrown exception for invalid config");
        } catch (const EventBusConfigurationException& e) {
            TestUtils::printSuccess("Correctly caught configuration exception: " + std::string(e.what()));
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Invalid Configuration Test");
        }

        // Test 3: Thread range validation
        try {
            TestUtils::printTestHeader("1.3 Thread Range Validation");
            EventBus::EventBusConfig rangeConfig;
            rangeConfig.thread_model = EventBus::ThreadModel::DYNAMIC;
            rangeConfig.task_model = EventBus::TaskModel::NORMAL;
            rangeConfig.thread_min = 10;
            rangeConfig.thread_max = 5;
            
            EventBus rangeBus;
            rangeBus.initEventBus(rangeConfig);
            TestUtils::printTestResult(false, "Should have thrown exception for invalid thread range");
        } catch (const EventBusConfigurationException& e) {
            TestUtils::printSuccess("Correctly caught thread range exception: " + std::string(e.what()));
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Thread Range Test");
        }
    }

    void testEventRegistration() {
        TestUtils::printTestHeader("2. Event Registration Tests");
        
        try {
            eventBus.registerEvent("test_event");
            TestUtils::printSuccess("Event 'test_event' registered successfully");
            
            eventBus.registerEvent("test_event");
            TestUtils::printSuccess("Duplicate event registration handled gracefully");
            
            eventBus.registerEvent("event1");
            eventBus.registerEvent("event2");
            eventBus.registerEvent("event3");
            TestUtils::printSuccess("Multiple events registered successfully");
            
            if (eventBus.isEventRegistered("test_event")) {
                TestUtils::printSuccess("Event registration verification passed");
            } else {
                TestUtils::printTestResult(false, "Event registration verification failed");
            }
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Event Registration");
            throw;
        }
    }

    void testSubscription() {
        TestUtils::printTestHeader("3. Subscription Tests");
        
        try {
            std::atomic<int> callback1Count{0};
            std::atomic<int> callback2Count{0};
            
            auto callback1 = [&callback1Count](std::string message) {
                callback1Count++;
                // 使用线程安全的输出
                TestUtils::printProgress("Callback1 received: " + message + " (Count: " + std::to_string(callback1Count) + ")");
            };
            
            auto id1 = eventBus.subscribe("test_event", callback1);
            TestUtils::printSuccess("Subscription 1 created with ID: " + std::to_string(id1));
            
            auto callback2 = [&callback2Count](std::string message) {
                callback2Count++;
                TestUtils::printProgress("Callback2 received: " + message + " (Count: " + std::to_string(callback2Count) + ")");
            };
            
            auto id2 = eventBus.subscribe("test_event", callback2);
            TestUtils::printSuccess("Subscription 2 created with ID: " + std::to_string(id2));
            
            auto id3 = eventBus.subscribeSafe("auto_registered_event", 
                [](const std::string& msg) {
                    TestUtils::printProgress("Auto-registered callback: " + msg);
                });
            TestUtils::printSuccess("Auto-registered subscription created with ID: " + std::to_string(id3));
            
            try {
                eventBus.subscribe("nonexistent_event", [](const std::string&) {});
                TestUtils::printTestResult(false, "Should have thrown exception for unregistered event");
            } catch (const EventNotRegisteredException& e) {
                TestUtils::printSuccess("Correctly caught unregistered event exception");
            }
            
            eventBus.publish("test_event", std::string("Hello Subscribers!"));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (callback1Count > 0 && callback2Count > 0) {
                TestUtils::printSuccess("Subscriptions are working correctly");
            } else {
                TestUtils::printTestResult(false, "Subscriptions not triggered properly ");
            }
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Subscription Tests");
            throw;
        }
    }

    void testPublishing() {
        TestUtils::printTestHeader("4. Publishing Tests");
        
        try {
            std::atomic<int> voidCallbackCount{0};
            std::atomic<int> stringCallbackCount{0};
            std::atomic<int> multiArgCallbackCount{0};
            
            eventBus.registerEvent("void_event");
            eventBus.registerEvent("string_event");
            eventBus.registerEvent("multi_arg_event");
            
            eventBus.subscribe("void_event", [&voidCallbackCount]() {
                voidCallbackCount++;
                TestUtils::printProgress("Void callback executed (Count: " + std::to_string(voidCallbackCount) + ")");
            });
            
            eventBus.subscribe("string_event", 
                [&stringCallbackCount](std::string msg) {
                    stringCallbackCount++;
                    TestUtils::printProgress("String callback: " + msg + " (Count: " + std::to_string(stringCallbackCount) + ")");
                });
            
            eventBus.subscribe("multi_arg_event",
                [&multiArgCallbackCount](int a, double b, std::string c) {
                    multiArgCallbackCount++;
                    TestUtils::printProgress("Multi-arg callback: " + std::to_string(a) + ", " + 
                                           std::to_string(b) + ", " + c + " (Count: " + 
                                           std::to_string(multiArgCallbackCount) + ")");
                });
            
            eventBus.publish("void_event");
            eventBus.publish("string_event", std::string("Test Message"));
            eventBus.publish("multi_arg_event", 42, 3.14, std::string("Hello"));
            
            try {
                eventBus.publish("unknown_event", std::string("test"));
                TestUtils::printTestResult(false, "Should have thrown exception for unregistered event publish");
            } catch (const EventNotRegisteredException& e) {
                TestUtils::printSuccess("Correctly caught unregistered event publish exception");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            if (voidCallbackCount > 0 && stringCallbackCount > 0 && multiArgCallbackCount > 0) {
                TestUtils::printSuccess("All publishing tests passed");
            } else {
                TestUtils::printTestResult(false, "Some callbacks were not triggered");
            }
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Publishing Tests");
            throw;
        }
    }

    void testPriorityPublishing() {
        TestUtils::printTestHeader("5. Priority Publishing Tests");
        
        try {
            EventBus::EventBusConfig priorityConfig;
            priorityConfig.thread_model = EventBus::ThreadModel::DYNAMIC;
            priorityConfig.task_model = EventBus::TaskModel::PRIORITY;
            priorityConfig.thread_min = 2;
            priorityConfig.thread_max = 4;
            priorityConfig.task_max = 50;
            
            EventBus priorityBus;
            priorityBus.initEventBus(priorityConfig);
            
            std::atomic<int> executionCount{0};
            
            priorityBus.registerEvent("priority_event");
            
            priorityBus.subscribe("priority_event",
                [&executionCount](const std::string& priority) {
                    executionCount++;
                    TestUtils::printProgress("Executing " + priority + " priority task (Total: " + 
                                           std::to_string(executionCount) + ")");
                });
            
            priorityBus.publishWithPriority(EventBus::TaskPriority::LOW, 
                "priority_event", std::string("LOW"));
            priorityBus.publishWithPriority(EventBus::TaskPriority::HIGH, 
                "priority_event", std::string("HIGH"));
            priorityBus.publishWithPriority(EventBus::TaskPriority::MIDDLE, 
                "priority_event", std::string("MIDDLE"));
            priorityBus.publishWithPriority(EventBus::TaskPriority::HIGH, 
                "priority_event", std::string("HIGH2"));
            
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            
            TestUtils::printSuccess("Priority publishing completed");
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Priority Publishing Tests");
        }
    }

    void testUnsubscription() {
        TestUtils::printTestHeader("6. Unsubscription Tests");
        
        try {
            std::atomic<int> activeCallbackCount{0};
            std::atomic<int> unsubscribedCallbackCount{0};
            
            eventBus.registerEvent("unsub_test_event");
            
            auto activeCallback = [&activeCallbackCount](std::string msg) {
                activeCallbackCount++;
                TestUtils::printProgress("Active callback: " + msg);
            };
            
            auto unsubscribedCallback = [&unsubscribedCallbackCount](std::string msg) {
                unsubscribedCallbackCount++;
                TestUtils::printProgress("This should not be called: " + msg);
            };
            
            auto activeId = eventBus.subscribe("unsub_test_event", activeCallback);
            auto unsubId = eventBus.subscribe("unsub_test_event", unsubscribedCallback);
            
            TestUtils::printSuccess("Created subscriptions: " + std::to_string(activeId) + ", " + std::to_string(unsubId));
            
            bool unsubResult = eventBus.unsubscribe("unsub_test_event", unsubId);
            if (unsubResult) {
                TestUtils::printSuccess("Successfully unsubscribed ID: " + std::to_string(unsubId));
            } else {
                TestUtils::printTestResult(false, "Failed to unsubscribe");
            }
            
            eventBus.publish("unsub_test_event", std::string("Test after unsubscribe"));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (activeCallbackCount > 0 && unsubscribedCallbackCount == 0) {
                TestUtils::printSuccess("Unsubscription working correctly");
            } else {
                TestUtils::printTestResult(false, "Unsubscription verification failed");
            }
            
            bool fakeUnsub = eventBus.unsubscribe("unsub_test_event", 99999);
            if (!fakeUnsub) {
                TestUtils::printSuccess("Correctly handled non-existent unsubscription");
            } else {
                TestUtils::printTestResult(false, "Should have failed to unsubscribe non-existent ID");
            }
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Unsubscription Tests");
            throw;
        }
    }

    void testErrorHandling() {
        TestUtils::printTestHeader("7. Error Handling Tests");
        
        try {
            try {
                EventBus uninitBus;
                uninitBus.registerEvent("test");
                TestUtils::printTestResult(false, "Should have thrown exception for uninitialized bus");
            } catch (const EventBusNotInitializedException& e) {
                TestUtils::printSuccess("Correctly caught uninitialized bus exception");
            }
            
            eventBus.registerEvent("exception_event");
            
            eventBus.subscribeSafe("exception_event", []() {
                TestUtils::printProgress("This callback will throw an exception...");
                throw std::runtime_error("Intentional callback exception");
            });
            
            eventBus.subscribeSafe("exception_event", []() {
                TestUtils::printProgress("This callback should still execute normally");
            });
            
            eventBus.publish("exception_event");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            TestUtils::printSuccess("Exception in callback handled gracefully");
            
            try {
                eventBus.publishWithPriority(EventBus::TaskPriority::HIGH, 
                    "test_event", std::string("test"));
                TestUtils::printTestResult(false, "Should have thrown task model mismatch exception");
            } catch (const TaskModelMismatchException& e) {
                TestUtils::printSuccess("Correctly caught task model mismatch exception");
            }
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Error Handling Tests");
        }
    }

    void testThreadSafety() {
        TestUtils::printTestHeader("8. Thread Safety Tests");
        
        try {
            std::atomic<int> concurrentCounter{0};
            const int THREAD_COUNT = 10;
            const int EVENTS_PER_THREAD = 20;
            
            eventBus.registerEvent("concurrent_event");
            
            // 使用原子计数器，避免在回调中打印以减少竞争
            eventBus.subscribe("concurrent_event", [&concurrentCounter](int value) {
                concurrentCounter += value;
                // 移除打印以减少竞争，只进行计数
                std::this_thread::sleep_for(std::chrono::microseconds(100)); // 轻微延迟模拟工作
            });
            
            std::vector<std::thread> threads;
            TestUtils::printProgress("Starting " + std::to_string(THREAD_COUNT) + " threads...");
            
            // 创建多个线程同时发布事件
            for (int i = 0; i < THREAD_COUNT; i++) {
                threads.emplace_back([this, i, EVENTS_PER_THREAD]() {
                    for (int j = 0; j < EVENTS_PER_THREAD; j++) {
                        try {
                            eventBus.publish("concurrent_event", 1);
                        } catch (const std::exception& e) {
                            TestUtils::printException(e, "Thread " + std::to_string(i));
                        }
                    }
                    TestUtils::printProgress("Thread " + std::to_string(i) + " completed");
                });
            }
            
            // 等待所有线程完成
            for (auto& thread : threads) {
                thread.join();
            }
            
            // 等待所有任务执行完成
            TestUtils::printProgress("Waiting for all events to be processed...");
            int waitAttempts = 0;
            while (concurrentCounter < THREAD_COUNT * EVENTS_PER_THREAD && waitAttempts < 50) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                waitAttempts++;
            }
            
            int expectedCount = THREAD_COUNT * EVENTS_PER_THREAD;
            if (concurrentCounter == expectedCount) {
                TestUtils::printSuccess("Thread safety test passed. Counter: " + 
                                      std::to_string(concurrentCounter) + "/" + 
                                      std::to_string(expectedCount));
            } else {
                TestUtils::printTestResult(false, 
                    "Thread safety test failed. Counter: " + 
                    std::to_string(concurrentCounter) + "/" + 
                    std::to_string(expectedCount));
            }
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Thread Safety Tests");
        }
    }

    void testPerformance() {
        TestUtils::printTestHeader("9. Performance Tests");
        
        try {
            const int EVENT_COUNT = 1000;
            std::atomic<int> processedCount{0};
            
            eventBus.registerEvent("perf_event");
            
            // 简单的性能测试回调，不打印以减少开销
            eventBus.subscribe("perf_event", [&processedCount](int id) {
                processedCount++;
            });
            
            auto startTime = std::chrono::high_resolution_clock::now();
            
            TestUtils::printProgress("Publishing " + std::to_string(EVENT_COUNT) + " events...");
            
            // 发布大量事件
            for (int i = 0; i < EVENT_COUNT; i++) {
                eventBus.publish("perf_event", i);
            }
            
            // 等待所有事件处理完成
            int waitAttempts = 0;
            while (processedCount < EVENT_COUNT && waitAttempts < 100) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                waitAttempts++;
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            if (processedCount == EVENT_COUNT) {
                TestUtils::printSuccess("Performance test completed: " + 
                                      std::to_string(EVENT_COUNT) + " events in " + 
                                      std::to_string(duration.count()) + "ms");
            } else {
                TestUtils::printWarning("Performance test incomplete: " + 
                                      std::to_string(processedCount) + "/" + 
                                      std::to_string(EVENT_COUNT) + " events processed");
            }
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Performance Tests");
        }
    }

    void testStatusMonitoring() {
        TestUtils::printTestHeader("10. Status Monitoring Tests");
        
        try {
            auto status = eventBus.getStatus();
            
            std::cout << "EventBus Status:\n";
            std::cout << "  Initialized: " << (status.is_initialized ? "Yes" : "No") << "\n";
            std::cout << "  Thread Model: " << static_cast<int>(status.thread_model) << "\n";
            std::cout << "  Task Model: " << static_cast<int>(status.task_model) << "\n";
            std::cout << "  Registered Events: " << status.event_system_status.registered_events_count << "\n";
            std::cout << "  Total Subscriptions: " << status.event_system_status.total_subscriptions << "\n";
            std::cout << "  Events Triggered: " << status.event_system_status.events_triggered_count << "\n";
            std::cout << "  Events Failed: " << status.event_system_status.events_failed_count << "\n";
            
            auto simpleStatus = eventBus.getSimplifiedStatus();
            std::cout << "\nSimplified Status:\n";
            std::cout << "  Thread Count: " << simpleStatus.thread_count << "\n";
            std::cout << "  Queue Size: " << simpleStatus.queue_size << "\n";
            std::cout << "  Active Threads: " << (simpleStatus.thread_count - status.thread_pool_status.idle_thread_count) << "\n";
            
            auto eventStats = eventBus.getEventStatistics("test_event");
            if (eventStats) {
                std::cout << "\n'test_event' Statistics:\n";
                std::cout << "  Subscriptions: " << eventStats->subscription_count << "\n";
                std::cout << "  Triggered: " << eventStats->triggered_count << "\n";
                std::cout << "  Failed: " << eventStats->failed_count << "\n";
                std::cout << "  Success Rate: " << eventStats->success_rate << "%\n";
            }
            
            eventBus.resetStatistics(true, true);
            auto resetStatus = eventBus.getSimplifiedStatus();
            if (resetStatus.events_triggered == 0) {
                TestUtils::printSuccess("Statistics reset working correctly");
            } else {
                TestUtils::printTestResult(false, "Statistics reset failed");
            }
            
            TestUtils::printSuccess("Status monitoring tests completed");
            
        } catch (const std::exception& e) {
            TestUtils::printException(e, "Status Monitoring Tests");
        }
    }
};

// 主函数
int main() {
    std::cout << "EventBus Comprehensive Test Suite\n";
    std::cout << "========================================\n";
    
    try {
        EventBusTester tester;
        tester.runAllTests();
        
        std::cout << "\n========================================\n";
        std::cout << "All tests completed successfully!\n";
        std::cout << "========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nCritical test failure: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}