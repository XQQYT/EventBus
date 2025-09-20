#include <iostream>
#include <thread>
#include <chrono>
#include "EventBus/EventBus.hpp"

class TestClass
{
public:
    void memberFunc(int a, int b)
    {
        std::cout << "Member function: a+b=" << a + b << std::endl;
    }
};

int main()
{
    TestClass obj;
    EventBus eventbus;
    EventBus::EventBusConfig config{EventBus::ThreadModel::DYNAMIC,EventBus::TaskModel::NORMAL,2,4,1024};
    eventbus.initEventBus(config);
    eventbus.registerEvent("MemberFunc");
    eventbus.subscribe("MemberFunc", std::bind(&TestClass::memberFunc, obj, std::placeholders::_1, std::placeholders::_2));
    eventbus.publish("MemberFunc", 77, 88);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
