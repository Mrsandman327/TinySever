#include "log.h"

#if _MSC_VER
#define vsnprintf _vsnprintf
#endif

log::log()
{
}


log::~log()
{
}

void log::initlog(const char* path, const char* exname, int maxqueuesize)
{
	struct ::tm tm_time;
	time_t timestamp;
	time(&timestamp);
#ifdef __linux__
	localtime_r(&timestamp, &tm_time);
#else
	localtime_s(&tm_time, &timestamp);
#endif
	std::ostringstream time_pid_stream;
	time_pid_stream.fill('0');
	time_pid_stream << 1900 + tm_time.tm_year
		<< std::setw(2) << 1 + tm_time.tm_mon
		<< std::setw(2) << tm_time.tm_mday
		<< '-'
		<< std::setw(2) << tm_time.tm_hour
		<< std::setw(2) << tm_time.tm_min
		<< std::setw(2) << tm_time.tm_sec
		<< '.'
		<< std::this_thread::get_id();
	const std::string& time_pid_string = time_pid_stream.str();

	char slogfile[1024];
	sprintf(slogfile, "%s/%s-%s.log", path, exname, time_pid_string.c_str());

	_logfile = slogfile;
	_maxqueuesize = maxqueuesize;

	/*_maxqueuesize > 0ʱʹ���첽ģʽ������ʹ��ͬ��*/
	if(_maxqueuesize)
		_pthreadpool = new ThreadPool(2, 100);
}

void log::writelog(int level, const char *_format, ...)
{
	char buffer[1024];
	memset(buffer, 0x00, sizeof(buffer));

	va_list ap;
	va_start(ap, _format);
	try
	{
		vsnprintf(buffer, 1024, _format, ap);
	}
	catch (...)
	{
		return;
	}
	va_end(ap);

	/*ʱ��*/
	time_t t = time(0);
	char timebuf[255];
	strftime(timebuf, 255, "%Y-%m-%d %H:%M:%S", localtime(&t));

	/*log����*/
	char const *slevel[4] = { "debug", "info", "warn", "erro" };

	/*log��Ϣ*/
	char infobuffer[2048];
	sprintf(infobuffer, "[%s:%s] %s", slevel[level], timebuf, buffer);

	std::string info = infobuffer;

	/*����log����*/
	_logqueue.push(info);

	if (_maxqueuesize)
	{
		/*�����̳߳�*/
		_pthreadpool->append([this]{ flushlog(); });
	}
	else
	{
		flushlog();
	}
}

void log::flushlog()
{
	if (_logqueue.size() <= 0)
	{
		return;
	}

	/*ȡ��log��Ϣ*/
	std::string info = _logqueue.front();
	_logqueue.pop();

	/*д��log�ļ�*/
	std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(_logfile, std::ios::out | std::ios::app);
	*content << info << std::endl;

	/*ǿ��ˢ�£�д����������*/
	_flushmutex.lock();
	content->flush();
	_flushmutex.unlock();
}

void log::uninitlog()
{
	if (_maxqueuesize)
		delete _pthreadpool;
}

