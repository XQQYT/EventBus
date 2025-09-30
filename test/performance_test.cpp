/*
 * EventBus Performance Monitor
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
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>
#include "EventBus/EventBus.hpp"

// ANSI 颜色代码
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define BOLDRED "\033[1;31m"
#define BOLDGREEN "\033[1;32m"
#define BOLDCYAN "\033[1;36m"

class PerformanceMonitor {
private:
    static std::mutex cout_mutex;

public:
    static void printHeader(const std::string& title) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << BOLDCYAN "<---->" << title << std::endl;
    }

    static void printMetric(const std::string& name, const std::string& value, const std::string& unit = "") {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << CYAN "  - " RESET << std::setw(25) << std::left << name 
                  << ": " GREEN << value;
        if (!unit.empty()) {
            std::cout << " " << unit;
        }
        std::cout << RESET "\n";
    }

    static void printProgress(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << BLUE "  [Progress] " RESET << message << "\n";
    }

    static void printWarning(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << YELLOW "  [Warning] " << message << RESET "\n";
    }

    static void printError(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << RED "  [Failed] " << message << RESET "\n";
    }

    static void printSuccess(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << GREEN "  [Successed] " << message << RESET "\n";
    }

    static void printSeparator() {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << CYAN "  -----------------------------------------------------" RESET "\n";
    }

    static std::string formatTime(long long milliseconds) {
        if (milliseconds < 1000) {
            return std::to_string(milliseconds) + "ms";
        } else if (milliseconds < 60000) {
            return std::to_string(milliseconds / 1000.0) + "s";
        } else {
            long long minutes = milliseconds / 60000;
            long long seconds = (milliseconds % 60000) / 1000;
            return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
        }
    }

    static std::string formatSize(size_t bytes) {
        const char* sizes[] = {"B", "KB", "MB", "GB"};
        int order = 0;
        double size = bytes;
        while (size >= 1024 && order < 3) {
            order++;
            size /= 1024;
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << sizes[order];
        return ss.str();
    }
};

std::mutex PerformanceMonitor::cout_mutex;

class EventBusPerformanceTester {
private:
    EventBus eventBus;
    std::vector<std::pair<std::string, uint64_t>> subscriptionIds; // 存储订阅ID用于清理

public:
    void runPerformanceTests() {
        PerformanceMonitor::printHeader("EVENTBUS PERFORMANCE BENCHMARK");
        
        try {
            // 初始化配置
            EventBus::EventBusConfig config;
            config.thread_model = EventBus::ThreadModel::DYNAMIC;
            config.task_model = EventBus::TaskModel::NORMAL;
            config.thread_min = 4;
            config.thread_max = 16;
            config.task_max = 1000000;
            
            eventBus.initEventBus(config);
            PerformanceMonitor::printSuccess("EventBus initialized for performance testing");

            // 运行各种性能测试
            testSingleEventThroughput();
            testMultipleEventsThroughput();
            testConcurrentPublishing();
            testLatency();
            testStressTest();
            testMixedWorkload();
            
            PerformanceMonitor::printHeader("PERFORMANCE TEST SUMMARY");
            printFinalSummary();
            
        } catch (const std::exception& e) {
            PerformanceMonitor::printError("Performance test failed: " + std::string(e.what()));
        }
    }

private:
    void testSingleEventThroughput() {
        PerformanceMonitor::printHeader("1. SINGLE EVENT THROUGHPUT TEST");
        
        const int EVENT_COUNTS[] = {1000, 10000, 50000};
        
        for (int event_count : EVENT_COUNTS) {
            PerformanceMonitor::printProgress("Testing with " + std::to_string(event_count) + " events...");
            
            std::atomic<int> processed{0};
            std::string event_name = "throughput_event_" + std::to_string(event_count);
            eventBus.registerEvent(event_name);
            
            // 存储订阅ID
            auto id = eventBus.subscribe(event_name, [&processed](int& value) {
                processed++;
            });
            subscriptionIds.emplace_back(event_name, id);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // 发布事件
            for (int i = 0; i < event_count; i++) {
                eventBus.publish(event_name, i);
            }
            
            // 等待处理完成
            waitForCompletion(processed, event_count, 5000);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double events_per_second = (event_count * 1000000.0) / duration.count();
            double avg_latency_us = duration.count() / (double)event_count;
            
            PerformanceMonitor::printMetric("Events Processed", std::to_string(processed) + "/" + std::to_string(event_count));
            PerformanceMonitor::printMetric("Total Time", PerformanceMonitor::formatTime(duration.count() / 1000));
            PerformanceMonitor::printMetric("Throughput", std::to_string((int)events_per_second) + " events/sec");
            PerformanceMonitor::printMetric("Avg Latency", std::to_string((int)avg_latency_us) + " us");
            PerformanceMonitor::printSeparator();
        }
    }

    void testMultipleEventsThroughput() {
        PerformanceMonitor::printHeader("2. MULTIPLE EVENTS THROUGHPUT TEST");
        
        const int EVENT_TYPES = 5;
        const int EVENTS_PER_TYPE = 2000;
        const int TOTAL_EVENTS = EVENT_TYPES * EVENTS_PER_TYPE;
        
        std::atomic<int> totalProcessed{0};
        
        // 注册多个事件类型
        for (int i = 0; i < EVENT_TYPES; i++) {
            std::string event_name = "multi_event_" + std::to_string(i);
            eventBus.registerEvent(event_name);
            
            auto id = eventBus.subscribe(event_name, [&totalProcessed](int& value) {
                totalProcessed++;
            });
            subscriptionIds.emplace_back(event_name, id);
        }
        
        PerformanceMonitor::printProgress("Testing " + std::to_string(EVENT_TYPES) + 
                                         " event types with " + std::to_string(TOTAL_EVENTS) + " total events...");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 发布混合事件
        for (int i = 0; i < EVENTS_PER_TYPE; i++) {
            for (int j = 0; j < EVENT_TYPES; j++) {
                eventBus.publish("multi_event_" + std::to_string(j), i);
            }
        }
        
        waitForCompletion(totalProcessed, TOTAL_EVENTS, 10000);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double events_per_second = (TOTAL_EVENTS * 1000000.0) / duration.count();
        
        PerformanceMonitor::printMetric("Total Events Processed", 
                                      std::to_string(totalProcessed) + "/" + std::to_string(TOTAL_EVENTS));
        PerformanceMonitor::printMetric("Event Types", std::to_string(EVENT_TYPES));
        PerformanceMonitor::printMetric("Total Time", PerformanceMonitor::formatTime(duration.count() / 1000));
        PerformanceMonitor::printMetric("Throughput", std::to_string((int)events_per_second) + " events/sec");
    }

    void testConcurrentPublishing() {
        PerformanceMonitor::printHeader("3. CONCURRENT PUBLISHING TEST");
        
        const int THREAD_COUNTS[] = {4, 8};
        const int EVENTS_PER_THREAD = 2000;
        
        for (int thread_count : THREAD_COUNTS) {
            PerformanceMonitor::printProgress("Testing with " + std::to_string(thread_count) + " threads...");
            
            std::atomic<int> totalProcessed{0};
            std::string event_name = "concurrent_event_" + std::to_string(thread_count);
            eventBus.registerEvent(event_name);
            
            auto id = eventBus.subscribe(event_name, [&totalProcessed](int& value) {
                totalProcessed++;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            });
            subscriptionIds.emplace_back(event_name, id);
            
            auto start = std::chrono::high_resolution_clock::now();
            
            std::vector<std::thread> threads;
            for (int i = 0; i < thread_count; i++) {
                threads.emplace_back([this, event_name, i, EVENTS_PER_THREAD]() {
                    for (int j = 0; j < EVENTS_PER_THREAD; j++) {
                        eventBus.publish(event_name, j);
                    }
                });
            }
            
            for (auto& thread : threads) {
                thread.join();
            }
            
            int totalEvents = thread_count * EVENTS_PER_THREAD;
            waitForCompletion(totalProcessed, totalEvents, 15000);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double events_per_second = (totalEvents * 1000000.0) / duration.count();
            
            PerformanceMonitor::printMetric("Threads", std::to_string(thread_count));
            PerformanceMonitor::printMetric("Events per Thread", std::to_string(EVENTS_PER_THREAD));
            PerformanceMonitor::printMetric("Total Events", std::to_string(totalEvents));
            PerformanceMonitor::printMetric("Processed", std::to_string(totalProcessed) + "/" + std::to_string(totalEvents));
            PerformanceMonitor::printMetric("Total Time", PerformanceMonitor::formatTime(duration.count() / 1000));
            PerformanceMonitor::printMetric("Throughput", std::to_string((int)events_per_second) + " events/sec");
            PerformanceMonitor::printSeparator();
        }
    }

    void testLatency() {
        PerformanceMonitor::printHeader("4. LATENCY MEASUREMENT TEST");
        
        const int SAMPLES = 500;
        std::vector<long long> latencies;
        latencies.reserve(SAMPLES);
        
        std::string event_name = "latency_event";
        eventBus.registerEvent(event_name);
        
        auto id = eventBus.subscribe(event_name, [&latencies](std::chrono::high_resolution_clock::time_point& start_time) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            latencies.push_back(latency.count());
        });
        subscriptionIds.emplace_back(event_name, id);
        
        PerformanceMonitor::printProgress("Measuring latency for " + std::to_string(SAMPLES) + " samples...");
        
        for (int i = 0; i < SAMPLES; i++) {
            auto start_time = std::chrono::high_resolution_clock::now();
            eventBus.publish(event_name, start_time);
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // 增加间隔避免队列过载
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        if (latencies.size() >= SAMPLES * 0.8) { // 至少80%的样本
            long long sum = 0;
            long long min_latency = std::numeric_limits<long long>::max();
            long long max_latency = 0;
            
            for (auto latency : latencies) {
                sum += latency;
                min_latency = std::min(min_latency, latency);
                max_latency = std::max(max_latency, latency);
            }
            
            double avg_latency = sum / (double)latencies.size();
            
            // 计算百分位数
            std::sort(latencies.begin(), latencies.end());
            long long p50 = latencies[latencies.size() * 0.5];
            long long p95 = latencies[latencies.size() * 0.95];
            long long p99 = latencies[latencies.size() * 0.99];
            
            PerformanceMonitor::printMetric("Samples Collected", std::to_string(latencies.size()));
            PerformanceMonitor::printMetric("Min Latency", std::to_string(min_latency) + " us");
            PerformanceMonitor::printMetric("Max Latency", std::to_string(max_latency) + " us");
            PerformanceMonitor::printMetric("Average Latency", std::to_string((int)avg_latency) + " us");
            PerformanceMonitor::printMetric("50th Percentile", std::to_string(p50) + " us");
            PerformanceMonitor::printMetric("95th Percentile", std::to_string(p95) + " us");
            PerformanceMonitor::printMetric("99th Percentile", std::to_string(p99) + " us");
        } else {
            PerformanceMonitor::printWarning("Insufficient samples collected: " + 
                                           std::to_string(latencies.size()) + "/" + std::to_string(SAMPLES));
        }
    }

    void testStressTest() {
        PerformanceMonitor::printHeader("5. STRESS TEST");
        
        const int STRESS_EVENTS = 100000;
        std::atomic<int> processed{0};
        std::string event_name = "stress_event";
        
        eventBus.registerEvent(event_name);
        
        auto id = eventBus.subscribe(event_name, [&processed](int& value) {
            processed++;
            // 中等工作负载
            volatile int result = 0;
            for (int i = 0; i < 50; i++) {
                result += i * i;
            }
        });
        subscriptionIds.emplace_back(event_name, id);
        
        PerformanceMonitor::printProgress("Running stress test with " + std::to_string(STRESS_EVENTS) + " events...");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 快速发布大量事件
        for (int i = 0; i < STRESS_EVENTS; i++) {
            eventBus.publish(event_name, i);
        }
        
        waitForCompletion(processed, STRESS_EVENTS, 30000);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        PerformanceMonitor::printMetric("Stress Events", std::to_string(STRESS_EVENTS));
        PerformanceMonitor::printMetric("Processed", std::to_string(processed) + "/" + std::to_string(STRESS_EVENTS));
        PerformanceMonitor::printMetric("Total Time", PerformanceMonitor::formatTime(duration.count()));
        PerformanceMonitor::printMetric("Events per Second", 
                                      std::to_string((int)(processed * 1000.0 / duration.count())) + " events/sec");
        
        // 检查系统状态
        auto status = eventBus.getStatus();
        PerformanceMonitor::printMetric("Events Failed", std::to_string(status.event_system_status.events_failed_count));
    }

    void testMixedWorkload() {
        PerformanceMonitor::printHeader("6. MIXED WORKLOAD TEST");
        
        const int EVENT_COUNT = 5000;
        std::atomic<int> fastProcessed{0};
        std::atomic<int> slowProcessed{0};
        std::atomic<int> mediumProcessed{0};
        
        // 使用不同的事件名称
        eventBus.registerEvent("fast_workload_event");
        eventBus.registerEvent("slow_workload_event");
        eventBus.registerEvent("medium_workload_event");
        
        // 快速回调（无延迟）
        auto fastId = eventBus.subscribe("fast_workload_event", [&fastProcessed](int& value) {
            fastProcessed++;
        });
        subscriptionIds.emplace_back("fast_workload_event", fastId);
        
        // 中等回调（轻微延迟）
        auto mediumId = eventBus.subscribe("medium_workload_event", [&mediumProcessed](int& value) {
            mediumProcessed++;
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        });
        subscriptionIds.emplace_back("medium_workload_event", mediumId);
        
        // 慢速回调（明显延迟）
        auto slowId = eventBus.subscribe("slow_workload_event", [&slowProcessed](int& value) {
            slowProcessed++;
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        });
        subscriptionIds.emplace_back("slow_workload_event", slowId);
        
        PerformanceMonitor::printProgress("Testing mixed workload with " + std::to_string(EVENT_COUNT * 3) + " total events...");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 发布混合事件
        for (int i = 0; i < EVENT_COUNT; i++) {
            eventBus.publish("fast_workload_event", i);
            eventBus.publish("medium_workload_event", i);
            eventBus.publish("slow_workload_event", i);
        }
        
        int totalEvents = EVENT_COUNT * 3;
        waitForCompletion([&]() {
            return fastProcessed + mediumProcessed + slowProcessed;
        }, totalEvents, 20000);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        PerformanceMonitor::printMetric("Fast Events", std::to_string(fastProcessed) + "/" + std::to_string(EVENT_COUNT));
        PerformanceMonitor::printMetric("Medium Events", std::to_string(mediumProcessed) + "/" + std::to_string(EVENT_COUNT));
        PerformanceMonitor::printMetric("Slow Events", std::to_string(slowProcessed) + "/" + std::to_string(EVENT_COUNT));
        PerformanceMonitor::printMetric("Total Time", PerformanceMonitor::formatTime(duration.count()));
        PerformanceMonitor::printMetric("Overall Throughput", 
                                      std::to_string((int)(totalEvents * 1000.0 / duration.count())) + " events/sec");
    }

    void printFinalSummary() {
        auto status = eventBus.getStatus();
        
        PerformanceMonitor::printMetric("Total Registered Events", std::to_string(status.event_system_status.registered_events_count));
        PerformanceMonitor::printMetric("Total Subscriptions", std::to_string(status.event_system_status.total_subscriptions));
        PerformanceMonitor::printMetric("Total Events Triggered", std::to_string(status.event_system_status.events_triggered_count));
        PerformanceMonitor::printMetric("Total Events Failed", std::to_string(status.event_system_status.events_failed_count));
        PerformanceMonitor::printMetric("Current Thread Count", std::to_string(status.thread_pool_status.thread_count));
        
        if (status.event_system_status.events_failed_count == 0) {
            PerformanceMonitor::printSuccess("All performance tests completed successfully!");
        } else {
            PerformanceMonitor::printWarning("Some events failed during performance testing");
        }
        
        cleanupSubscriptions();
    }

private:
    void waitForCompletion(std::atomic<int>& counter, int expected, int max_wait_ms) {
        int wait_attempts = 0;
        int max_attempts = max_wait_ms / 10;
        
        while (counter < expected && wait_attempts < max_attempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            wait_attempts++;
            
            if (wait_attempts % 20 == 0) { // 每200ms报告一次进度
                PerformanceMonitor::printProgress("Waiting... " + std::to_string(counter) + "/" + std::to_string(expected) + " processed");
            }
        }
    }
    
    void waitForCompletion(std::function<int()> get_count, int expected, int max_wait_ms) {
        int wait_attempts = 0;
        int max_attempts = max_wait_ms / 10;
        
        while (get_count() < expected && wait_attempts < max_attempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            wait_attempts++;
            
            if (wait_attempts % 20 == 0) {
                int current = get_count();
                PerformanceMonitor::printProgress("Waiting... " + std::to_string(current) + "/" + std::to_string(expected) + " processed");
            }
        }
    }

    void cleanupSubscriptions() {
        PerformanceMonitor::printProgress("Cleaning up subscriptions...");
        int removed_count = 0;
        
        for (const auto& [event_name, subscription_id] : subscriptionIds) {
            if (eventBus.unsubscribe(event_name, subscription_id)) {
                removed_count++;
            }
        }
        
        PerformanceMonitor::printSuccess("Cleaned up " + std::to_string(removed_count) + " subscriptions");
        subscriptionIds.clear();
    }
};

int main() {
    PerformanceMonitor::printHeader("EVENTBUS PERFORMANCE MONITOR");
    std::cout << "Starting comprehensive performance analysis...\n\n";
    
    try {
        EventBusPerformanceTester tester;
        tester.runPerformanceTests();
        
        std::cout << "\n" BOLDGREEN "Performance monitoring completed successfully!" RESET "\n";
        
    } catch (const std::exception& e) {
        PerformanceMonitor::printError("Performance monitoring failed: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}