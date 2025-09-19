cmake_minimum_required(VERSION 3.2.0)
project(EventBus)

add_executable(NormalFuncTest example/NormalFunc.cpp)
target_include_directories(NormalFuncTest PRIVATE include)

add_executable(LambdaTest example/Lambda.cpp)
target_include_directories(LambdaTest PRIVATE include)

add_executable(MemberFunc example/MemberFunc.cpp)
target_include_directories(MemberFunc PRIVATE include)