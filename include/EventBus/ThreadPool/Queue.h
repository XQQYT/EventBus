/*
 * EventBus
 * Author: XQQYT
 * License: MIT
 * Year: 2025
 */

#ifndef  _QUEUE_H
#define _QUEUE_H
#include <iostream>
#include <queue>
#include <semaphore.h>
#include <mutex>
//#include <shared_mutex>
#include <functional>



template <class... Args>
class Queue
{
public:
	Queue() {};
	virtual ~Queue() {};
    virtual void addTask(std::function<void(Args...)>&& func, Args&&... args){};
    virtual void addTask(int priority, std::function<void(Args...)>&& func, Args&&... args){};
	virtual std::pair<std::function<void(Args...)>, std::tuple<Args...>> getTask() = 0;

	virtual inline int getCapacity() noexcept = 0;

	virtual inline int getSize() noexcept = 0;

	virtual void release() = 0;

	virtual void acquire() = 0;
private:

};







#endif // ! _QUEUE_H
