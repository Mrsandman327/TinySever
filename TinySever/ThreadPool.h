#pragma once
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>

typedef std::function<void()> callback;
class ThreadPool
{
public:
	ThreadPool(int threadnum = 1, int maxwork = 10000);
	~ThreadPool();
public:
	std::vector<std::thread> _pool;								/*�̳߳�*/
public:
	bool append(callback func);
private:
	bool _threadrun;
	int _threadnum;												/*�̳߳��е��߳���*/
	int _maxwork;												/*�������*/
	std::mutex _mutex;
	std::condition_variable _conv;
	std::queue<callback> _workqueue;
private:									
	void run();													/*�����߳����еĺ����������ϴӹ���������ȡ������ִ��֮*/
};
