#pragma once
#include <memory>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "ThreadPool.h"
class log
{
public:
	static log &getInstance()
	{
		static log instance;
		return instance;
	}
private:
	log();
	~log();
	std::string _logfile;
	int _maxqueuesize;
	std::queue<std::string> _logqueue;
	std::mutex _flushmutex;
	ThreadPool *_pthreadpool;
public:
	void initlog(const char* path, const char* exname, int maxqueuesize = 100);
	void writelog(int level, const char *_format, ...);
	void flushlog();
	void uninitlog();
};

#define LOG_INIT(path, exname, size)log::getInstance().initlog(path, exname, size);
#define LOG_DEBUG(fmt, ...)			log::getInstance().writelog(0,"[%s]"fmt,__FILE__, ##__VA_ARGS__);
#define LOG_INFO(fmt, ...)			log::getInstance().writelog(1,"[%s]"fmt,__FILE__, ##__VA_ARGS__);
#define LOG_WARING(fmt, ...)		log::getInstance().writelog(2,"[%s]"fmt,__FILE__, ##__VA_ARGS__);
#define LOG_ERROR(fmt, ...)			log::getInstance().writelog(3,"[%s]"fmt,__FILE__, ##__VA_ARGS__);
#define LOG_UNINIT					log::getInstance().uninitlog();