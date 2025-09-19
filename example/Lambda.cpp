#include <iostream>
#include <thread>
#include <chrono>
#include "../include/EventBus.h"

int main(){
    EventBus::getInstance().registerEvent("LambdaTest");
    EventBus::getInstance().subscribe("LambdaTest",[](int a,int b){
        std::cout<<"LambdaTest: a+b="<<a+b<<std::endl;
    });
    EventBus::getInstance().publish("LambdaTest",77,88);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}