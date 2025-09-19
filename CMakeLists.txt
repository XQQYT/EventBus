cmake_minimum_required(VERSION 3.2.0)
project(EventBus)

add_executable(NormalFuncTest test/NormalFunc.cpp)
target_include_directories(NormalFuncTest PRIVATE ThreadPool)

add_executable(LambdaTest test/Lambda.cpp)
target_include_directories(LambdaTest PRIVATE ThreadPool)

add_executable(MemberFunc test/MemberFunc.cpp)
target_include_directories(MemberFunc PRIVATE ThreadPool)