cmake_minimum_required(VERSION 3.2.0)
project(EventBus)

function(add_example name)
    add_executable(${name} example/${name}.cpp)
    target_include_directories(${name} PRIVATE include)
endfunction()

add_example(NormalFunc)
add_example(Lambda)
add_example(MemberFunc)
