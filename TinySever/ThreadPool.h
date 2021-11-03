/*******************************************************************************
* @file     ThreadPool.h
* @brief    threadpool
* @author   linsn
* @date:    2021-9-16
******************************************************************************/
#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>

typedef std::function<void()> callback;
class ThreadPool
{
public:
	ThreadPool(int threadnum = 1, int maxwork = 10000);
	~ThreadPool();
public:
	std::vector<std::thread> _pool;								/*线程池*/
public:
	bool append(callback func);
private:
	std::atomic<bool> _threadrun;
	int _threadnum;												/*线程池中的线程数*/
	int _maxwork;												/*最大工作数*/
	std::mutex _mutex;
	std::condition_variable _conv;
	std::queue<callback> _workqueue;
private:									
	void run();													/*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
};
