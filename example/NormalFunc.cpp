#include <iostream>
#include <thread>
#include <chrono>
#include "EventBus/EventBus.hpp"
#include <ctime>
#include <mutex>

std::mutex mtx;

void func(int a,int b){
    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout<<"Normal function: a+b="<<a+b<<std::endl;
    }
    std::srand(static_cast<unsigned int>(std::time(0)));
    int random_num = (std::rand() % 4) + 1;
    std::this_thread::sleep_for(std::chrono::seconds(random_num));
}

int main(){
    EventBus eventbus;
    EventBus::EventBusConfig config{EventBus::ThreadModel::DYNAMIC,EventBus::TaskModel::PRIORITY,2,4,1024};
    try{
        eventbus.initEventBus(config);
    }catch(const std::exception& e){
        std::cout<<e.what()<<std::endl;
    }
    eventbus.registerEvent("NormalFuncTest");
    eventbus.subscribe("NormalFuncTest",func);
    eventbus.publishWithPriority(EventBus::TaskPriority::LOW,"NormalFuncTest",1,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::LOW,"NormalFuncTest",2,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::LOW,"NormalFuncTest",3,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::LOW,"NormalFuncTest",4,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::LOW,"NormalFuncTest",5,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::LOW,"NormalFuncTest",6,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::HIGH,"NormalFuncTest",100,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::HIGH,"NormalFuncTest",99,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::HIGH,"NormalFuncTest",98,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::HIGH,"NormalFuncTest",97,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::HIGH,"NormalFuncTest",96,0);
    eventbus.publishWithPriority(EventBus::TaskPriority::HIGH,"NormalFuncTest",95,0);
    std::this_thread::sleep_for(std::chrono::seconds(20));
    return 0;
}