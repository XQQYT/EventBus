#include <iostream>
#include <thread>
#include <chrono>
#include "EventBus/EventBus.h"

int main(){
    EventBus eventbus;
    EventBus::EventBusConfig config{EventBus::ThreadModel::DYNAMIC,2,4,1024};
    eventbus.initEventBus(config);
    eventbus.registerEvent("LambdaTest");
    eventbus.subscribe("LambdaTest",[](int a,int b){
        std::cout<<"LambdaTest: a+b="<<a+b<<std::endl;
    });
    eventbus.publish("LambdaTest",77,88);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}