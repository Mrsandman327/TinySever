#include "ThreadPool.h"

ThreadPool::ThreadPool(int threadnum, int maxwork) :
	_threadnum(threadnum),
	_maxwork(maxwork),
	_threadrun(true)
{
	if (_threadnum > 0 && _maxwork > 0)
	{
		for (int i = 0; i < _threadnum; ++i)
		{
			//std::thread thread(&ThreadPool::run, this);
			//thread.detach();
			///*thread���ɸ��ƣ����ǿ����ƶ�*/
			//_pool.push_back(std::move(thread));

			_pool.emplace_back([this]{run(); });
			
		}
	}
}

ThreadPool::~ThreadPool()
{
	_threadrun = false;
	_conv.notify_all();

	/* �ȴ���������� ǰ�᣺�߳�һ����ִ���� */
	for (std::thread& thread : _pool)
	{
		if (thread.joinable())
			thread.join();
	}
	_pool.clear();
}

bool ThreadPool::append(callback func)
{
	if (static_cast<int>(_workqueue.size()) >= _maxwork)
	{
		return false;
	}

	do{
		std::lock_guard<std::mutex> lock(_mutex);
		_workqueue.push(func);
	} while (0);

	_conv.notify_one();

	return true;
}

void ThreadPool::run()
{
	while (_threadrun)
	{
		std::unique_lock<std::mutex> lock(_mutex);
		/*ѭ���жϣ���ֹ�ٻ���*/
		while (_workqueue.size() == 0)
		{
			_conv.wait(lock);
		}
		if (!_threadrun)
			break;

		callback func = _workqueue.front();
		_workqueue.pop();

		func();
	}
}