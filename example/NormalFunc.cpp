#include <iostream>
#include <thread>
#include <chrono>
#include "EventBus/EventBus.h"

void func(int a,int b){
    std::cout<<"Normal function: a+b="<<a+b<<std::endl;
}

int main(){
    EventBus eventbus;
    EventBus::EventBusConfig config{EventBus::ThreadModel::DYNAMIC,2,4,1024};
    eventbus.initEventBus(config);
    eventbus.registerEvent("NormalFuncTest");
    eventbus.subscribe("NormalFuncTest",func);
    eventbus.publish("NormalFuncTest",77,88);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}