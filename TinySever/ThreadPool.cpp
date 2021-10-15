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
			///*thread不可复制，但是可以移动*/
			//_pool.push_back(std::move(thread));

			_pool.emplace_back([this]{run(); });
			
		}
	}
}

ThreadPool::~ThreadPool()
{
	_threadrun = false;
	_conv.notify_all();

	/* 等待任务结束， 前提：线程一定会执行完 */
	for (std::thread& thread : _pool)
	{
		if (thread.joinable())
			thread.join();
	}

	if (!_workqueue.empty())
	{
		std::queue<callback> empty;
		swap(empty, _workqueue);
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
		printf("开始执行，threadid:%d\n", std::this_thread::get_id());
		std::unique_lock<std::mutex> lock(_mutex);
#if 0
		/*循环判断，防止假唤醒*/
		while (_workqueuee.empty() && _threadrun)
		{
			_conv.wait(lock);
		}
#else 
		/*wait函数有第二个参数也能防止假唤醒*/
		_conv.wait(lock, [this]{
			if (_workqueue.empty() && _threadrun)
				return false;
			return true;
		});
#endif	

		if (!_threadrun)
			break;

		if (!_workqueue.empty())
		{ 
			callback func = _workqueue.front();
			_workqueue.pop();

			func();
		}
	}
}