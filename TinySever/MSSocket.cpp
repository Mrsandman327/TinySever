#include "MSSocket.h"
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>

#ifdef __linux__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> //close()
#include <netdb.h> //gethostbyname
#include <fcntl.h>

#define INVALID_SOCKET  (unsigned int)(~0)
#define SOCKET_ERROR				  (-1)
#define closesocket close
#define SD_RECEIVE 	SHUT_RD
#define SD_SEND 	SHUT_WD
#define SD_BOTH 	SHUT_RDWR
#define WOULDBLOCK  EWOULDBLOCK

#elif  defined(_WIN32)
#include <winsock2.h>  
#pragma comment(lib,"ws2_32.lib") 
#define WOULDBLOCK  WSAEWOULDBLOCK
#define socklen_t int
#endif

#define _Blocking			0	/*堵塞I/O*/
#define _NonBlocking		1   /*非堵塞I/O(非堵塞模式下使用多路IO复用模型，window下使用select,linux下可选择select,epoll)*/
#define _IOMODEL_  _NonBlocking

#define READTHREAD 4
#define MAXREQUEST 10000

CMSSocket::CMSSocket()
{
	init_skt();
}

CMSSocket::~CMSSocket()
{
	uninit_skt();
}

void CMSSocket::attach_observable(CSocketObservable *observer)
{
	_observerlist.push_back(observer);
}

void CMSSocket::dttach_observable(CSocketObservable* observer)
{
	for (auto it = _observerlist.begin(); it != _observerlist.end(); it++)
	{
		if (*it == observer)
		{
			_observerlist.remove(observer);
			break;
		}
	}
}

void CMSSocket::notify_observable(int socket)
{
	for (auto it = _observerlist.begin(); it != _observerlist.end(); it++)
	{
		(*it)->Update(socket);
	}
}

void CMSSocket::addclientsock(int s)
{
	_clientsocklist.push_back(s);
}

void CMSSocket::delclientsock(int s)
{
	int size = _clientsocklist.size();
	for (int i = 0; i < size; ++i)
	{
		if (_clientsocklist[i] == s)
		{
			_clientsocklist.erase(_clientsocklist.begin() + i);
			break;
		}
	}
}

int CMSSocket::geterror_skt()
{
#ifdef __linux__
	return errno;
#elif  defined(_WIN32)
	return GetLastError();
#endif

}

int CMSSocket::init_skt()
{
#ifdef _WIN32
	unsigned short sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if ((WSAStartup(WS_VERSION_CHOICE1, &wsaData) != 0) &&
		((WSAStartup(WS_VERSION_CHOICE2, &wsaData)) != 0))
	{
		printf("socket---WSAStartup error,code:%d\n", geterror_skt());
		return -1;
	}
	if ((wsaData.wVersion != WS_VERSION_CHOICE1) &&
		(wsaData.wVersion != WS_VERSION_CHOICE2))
	{
		printf("socket---WSAStartup error,code:%d\n", geterror_skt());
		WSACleanup();
		return -1;
	}
#endif
	return 0;
}

int CMSSocket::uninit_skt()
{
#ifdef _WIN32
	WSACleanup();
#endif
	return 1;
}

int CMSSocket::make_skt()
{
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (s == INVALID_SOCKET) {
		printf("socket---socket error,code:%d\n", geterror_skt());
		return SOCKET_ERROR;
	}
	return s;
}

void CMSSocket::close_skt(int s)
{
	if (s) 
	{
		closesocket(s);
		s = 0;
	}
}

void CMSSocket::clientclose(int s)
{
	/* 关闭发送和接收操作 */
	shutdown(s, SD_BOTH);
	close_skt(s);
	delclientsock(s);
}

void CMSSocket::severclose(int s)
{
	shutdown(s, SD_BOTH);
	close_skt(s);

	for (auto it = _clientsocklist.begin(); it != _clientsocklist.end(); it++)
	{
		shutdown(*it, SD_BOTH);
		close_skt(*it);
	}
	_clientsocklist.clear();
}

void CMSSocket::setnoblocking_skt(int s)
{
#ifdef _WIN32
	unsigned long ul = 1;
	ioctlsocket(s, FIONBIO, (unsigned long *)&ul);
#else
	int flags = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
}

void CMSSocket::addepollfd_skt(int epollfd, int fd, bool oneshot)
{
#ifdef __linux__
	epoll_event event;
	event.data.fd = fd;

	/*设置要处理的事件类型(可读事件，边沿触发)*/
	event.events = EPOLLIN | EPOLLET;
	/*采用EPOLLONETSHOT事件*/
	if (oneshot)
	{
		event.events |= EPOLLONESHOT;
	}

	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
#endif
}

void CMSSocket::delepollfd_skt(int epollfd, int fd, bool oneshot)
{
#ifdef __linux__
	epoll_event event;
	event.data.fd = fd;

	/*设置要处理的事件类型(可读事件，边沿触发)*/
	event.events = EPOLLIN | EPOLLET;

	/*采用EPOLLONETSHOT事件*/
	if (oneshot)
	{
		event.events |= EPOLLONESHOT;
	}

	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &event);
#endif
}

void CMSSocket::resetonshot_skt(int epollfd, int fd)
{
#ifdef __linux__
	epoll_event event;
	event.data.fd = fd;

	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
#endif
}

int CMSSocket::send_skt(int s, char *data, int len)
{
	int length;
	if ((length = send(s, data, len, 0)) == SOCKET_ERROR)
	{
		printf("socket---send error,code:%d\n", geterror_skt());
		if (geterror_skt() == WOULDBLOCK)
			return 0;
		close_skt(s);
		return -1;
	}
	else if (length != len) {
		close_skt(s);
		return -1;
	}
	return 0;
}

int CMSSocket::receive_skt(int s, char *data, int len)
{
	if ((len = recv(s, data, len, 0)) == SOCKET_ERROR) 
	{
		printf("socket---recv error,code:%d\n", geterror_skt());
		close_skt(s);
		return len;
	}
	if (len == 0)
	{
		close_skt(s);
	}
	return len;
}

bool CMSSocket::connect_skt(int s, std::string addr, int port)
{
	unsigned long lAddr = inet_addr(addr.c_str());

	if (lAddr == INADDR_NONE)
	{
		hostent *h = gethostbyname(addr.c_str());/*查看是否是域名*/
		if (h == NULL)
			return false;
		else
			lAddr = *((unsigned long *)(h->h_addr));
	}   

	sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = lAddr;
	sockAddr.sin_port = htons(port);

	if (connect(s, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		printf("socket---connect error,code:%d\n", geterror_skt());
		close_skt(s);
		return false;
	}
	return true;
}

bool CMSSocket::listen_skt(int s, std::string addr, int port)
{
	unsigned long lAddr = inet_addr(addr.c_str());

	if (addr.empty())
		lAddr = htonl(INADDR_ANY);

	if (lAddr == INADDR_NONE)
	{
		hostent *h = gethostbyname(addr.c_str());
		if (h == nullptr)
			lAddr = htonl(INADDR_ANY);
		else
			lAddr = *((unsigned long *)(h->h_addr));
	}

	/*允许重用本地地址和端口*/
	int breuseaddr = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&breuseaddr, sizeof(int));

	/*设置非堵塞模式*/
#if _IOMODEL_ == _NonBlocking
	setnoblocking_skt(s);
#endif

	sockaddr_in sockAddr;
	//memset(sockAddr.sin_zero, 0, sizeof(struct sockaddr_in));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = lAddr;
	sockAddr.sin_port = htons(port);

	if (bind(s, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		printf("socket---bind error,code:%d\n", geterror_skt());
		return false;
	}

	if (listen(s, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("socket---listen error,code:%d\n", geterror_skt());
		return false;
	}

	_pthreadpool = new ThreadPool(READTHREAD, MAXREQUEST);

#if _IOMODEL_ == _NonBlocking
#ifdef __linux__
	std::thread thread(&CMSSocket::serverepoll_skt, this, s);
	thread.detach();
#else
	std::thread thread(&CMSSocket::serverselect_skt, this, s);
	thread.detach();
#endif
#elif _IOMODEL_ == _Blocking
	std::thread thread(&CMSSocket::accpet_skt, this, s);
	thread.detach();
#endif
	return true;
}

void CMSSocket::serverselect_skt(int s)
{
	/*连接的客户端信息*/
	struct sockaddr_in clientaddr;
	int addrlen = sizeof(clientaddr);

	/*保存最大文件描述符*/
	int maxfd = s;

	/*文件描述符*/
	fd_set readfds;

	/*把readfds清空*/
	FD_ZERO(&readfds);

	/*把要监听的sockfd添加到readfds中*/
	FD_SET(s, &readfds);
	maxfd = (maxfd > s ? maxfd : s) + 1;

	while (true)
	{
		/*用select类监听sockfd 阻塞状态*/
		int  ret = select(maxfd, &readfds, NULL, NULL, NULL);
		
		if (ret == -1)
		{
			printf("socket---select error,code:%d\n", geterror_skt());
			break;
		}
		else if (ret == 0)
		{
			printf("socket---select timeout\n");
			continue;
		}

		/*判断是否是客户端链接*/
		if (FD_ISSET(s, &readfds))
		{
			/* 接受客户端连接*/
			int clientfd = accept(s, (struct sockaddr *)&clientaddr, (socklen_t *)&addrlen);
			if (clientfd != INVALID_SOCKET)
			{
				/*添加到客户socket列表*/
				addclientsock(clientfd);

				/*客户端套接字描述符添加到监听集合中*/
				FD_SET(clientfd, &readfds);
				maxfd = (maxfd > clientfd ? maxfd : clientfd) + 1;

				/*设置接入的连接为非堵塞*/
				setnoblocking_skt(clientfd);

				/* 通知订阅者 */
				setsocketevent(clientaccpet);
				notify_observable(clientfd);
				//printf("ip=%s\n", inet_ntoa(clientaddr.sin_addr));

				continue;
			}
			else
			{
				/*服务器关闭*/
				delete _pthreadpool;
				break;
			}
		}
		else
		{
			/*select函数返回时, readfds里只会留下可以读操作的文件描述符，
			其它不可操作的文件描述符会移除掉的。这里需要重新添加*/
			FD_SET(s, &readfds);
			maxfd = (maxfd > s ? maxfd : s) + 1;
		}

		int clientsize = getclientsocksize();
		for (int i = 0; i < clientsize; i++)
		{
			if (FD_ISSET(_clientsocklist[i], &readfds))
			{
				/* 读取数据*/
				char rbuffer[65535];
				int datalen = receive_skt(_clientsocklist[i], rbuffer, sizeof(rbuffer));
				if (datalen == 0)
				{
					continue;
				}
				else if (datalen == SOCKET_ERROR)
				{
					/*客户端套接字描述符从监听集合中删除*/
					FD_CLR(_clientsocklist[i], &readfds);
					
					/* 通知订阅者 */
					setsocketevent(clientdiscon);
					notify_observable(_clientsocklist[i]);
					
					/*关闭客户端*/
					clientclose(_clientsocklist[i]);
					--i;--clientsize;

					/*计算描述符最大个数*/
					maxfd = *max_element(_clientsocklist.begin(), _clientsocklist.end()) + 1;
					//break;
				}
				else
				{
					recvdata data;
					data.socket = _clientsocklist[i];
					memcpy(&data.buffer, rbuffer, DATAPACKETSIZE);
					std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
					_dataqueue.push(recvda);

					/* 通知订阅者 */
					if (datalen == DATAPACKETSIZE)
						setsocketevent(serverrecv);
					else
						setsocketevent(datanodefine);
					//notify_observable(_clientsocklist[i]);
					callback func = std::bind(&CMSSocket::notify_observable, this, _clientsocklist[i]);
					_pthreadpool->append(func);
				}
			}
			else
			{
				/*select函数返回时, readfds里只会留下可以读操作的文件描述符，
				其它不可操作的文件描述符会移除掉的。这里需要重新添加*/
				FD_SET(_clientsocklist[i], &readfds);
				maxfd = (maxfd > _clientsocklist[i] ? maxfd : _clientsocklist[i]) + 1;
			}
		}
	}
}

void CMSSocket::serverepoll_skt(int s)
{
#ifdef __linux__
	/*声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件*/
	struct epoll_event events[10000];

	/*生成用于处理accept的epoll专用的文件描述符*/
	int epfd = epoll_create(256);
	if(epfd == -1)
	{
		printf("socket---epoll_create error,code:%d\n", geterror_skt());
		return false;
	}

	/*注册epoll事件*/
	addepollfd_skt(epfd, s, true);

	while (true)
	{
		/*等待监控事件发生 -1：代表无限等待*/
		int nfds = epoll_wait(epfd, events, 10000, -1);
		if (nfds == -1)
		{
			if(geterror_skt() != EINTR)
			{
				printf("socket---epoll_wait error,code:%d\n", geterror_skt());
				delete _pthreadpool;
				break;
			}
			else
			{
				/*代码中忽略由于接收调试信号而产生的"错误"返回*/
				continue;
			}
		}
		else if (nfds == 0)
		{
			printf("socket---epoll_wait timeout\n");
			continue;
		}

		for (int i = 0; i < nfds; ++i)
		{
			int sockfd = events[i].data.fd;
			if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				/* 监控到错误或者挂起 */
				printf("epoll error\n");
				/* 通知订阅者 */
				setsocketevent(clientdiscon);
				notify_observable(sockfd);

				clientclose(sockfd);
				continue;
			} 
			else if (sockfd == s) /*有新的连接*/
			{
				/*连接的客户端信息*/
				struct sockaddr_in clientaddr;
				int addrlen = sizeof(clientaddr);
				int clientfd = accept(s, (struct sockaddr *)&clientaddr, (socklen_t *)&addrlen);; //accept这个连接
				if (clientfd != INVALID_SOCKET)
				{
					/*添加到客户socket列表*/
					addclientsock(clientfd);

					/*设置接入的连接为非堵塞*/
					setnoblocking_skt(clientfd);

					/* 通知订阅者 */
					setsocketevent(clientaccpet);
					notify_observable(clientfd);

					/*将新的fd添加到epoll的监听队列中*/
					addepollfd_skt(epfd,clientfd,true);

					resetonshot_skt(epfd,s);
				}
			}
			else if (events[i].events & EPOLLIN)/*有可读文件*/
			{
				if (sockfd  < 0)
					continue;

				char buf[65535];
				int datalen = receive_skt(sockfd, buf, sizeof(buf));
				if (datalen == 0)
				{
					continue;
				}
				else if (datalen == SOCKET_ERROR)
				{
					if (geterror_skt() != EAGAIN)
					{

						/*客户端套接字描述符从监听集合中删除*/
						delepollfd_skt(epfd, sockfd, true);

						/* 通知订阅者 */
						setsocketevent(clientdiscon);
						notify_observable(sockfd);

						clientclose(sockfd);
					}
					else
					{
						resetonshot_skt(epfd, sockfd);
						continue;
					}
					//break;
				}
				else
				{
					recvdata data;
					memcpy(&data, buf, DATAPACKETSIZE);
					std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
					_dataqueue.push(recvda);

					/* 通知订阅者 */
					if (datalen == DATAPACKETSIZE)
						setsocketevent(serverrecv);
					else
						setsocketevent(datanodefine);
					//notify_observable(sockfd);
					callback func = std::bind(&CMSSocket::notify_observable, this, sockfd);
					_pthreadpool->append(func);	

					resetonshot_skt(epfd, sockfd);
				}
			}
		}
	}

#endif
}

void CMSSocket::accpet_skt(int s)
{
	struct sockaddr_in sockAddr;
	int addrlen = sizeof(sockAddr);
	while (true)
	{
		/*堵塞，等待客户端连接*/
		int sockfd = accept(s, (struct sockaddr *)&sockAddr, (socklen_t *)&addrlen);
		if (sockfd == INVALID_SOCKET)
		{
			break;
		}
		else
		{
			/*
			struct sockaddr_in clientAddr;
			int clientAddrLen = sizeof(clientAddr);
			//获取sockfd表示的连接上的本地地址
			getsockname(sockfd, (struct sockaddr*)&clientAddr, &clientAddrLen);

			int prot = ntohs(clientAddr.sin_port);
			std::string ip = inet_ntoa(clientAddr.sin_addr);
			std::string iP = inet_ntoa(sockAddr.sin_addr);
			*/

			/*添加到客户socket列表*/
			addclientsock(sockfd);

			/* 通知订阅者 */
			setsocketevent(clientaccpet);
			notify_observable(sockfd);

			std::thread thread(&CMSSocket::severreceive_skt, this, sockfd);
			thread.detach();
		}
	}
	closesocket(s);
}

void CMSSocket::clientreceive_skt(int s)
{
	char	buf[65535];
	while (true)
	{
		int datalen = receive_skt(s, buf, sizeof(buf));
		if (datalen == 0)
		{
			continue;
		}
		else if (datalen == SOCKET_ERROR)
		{
			break;
		}
		else
		{
			recvdata data;
			memcpy(&data, buf, DATAPACKETSIZE);
			std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
			_dataqueue.push(recvda);

			/* 通知订阅者 */
			if (datalen == DATAPACKETSIZE)
				setsocketevent(clientrecv);
			else
				setsocketevent(datanodefine);
			notify_observable(s);
		}
	}
	clientclose(s);
	/* 通知订阅者 */
	setsocketevent(clientdiscon);
	notify_observable(s);
}

void CMSSocket::severreceive_skt(int s)
{
	char	buf[65535];
	while (true)
	{
		int datalen = receive_skt(s, buf, sizeof(buf));
		if (datalen == 0)
		{
			continue;
		}
		else if (datalen == SOCKET_ERROR)
		{
			break;
		}
		else
		{
			recvdata data;
			memcpy(&data, buf, DATAPACKETSIZE);
			std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
			_dataqueue.push(recvda);

			/* 通知订阅者 */
			if (datalen == DATAPACKETSIZE)
				setsocketevent(serverrecv);
			else
				setsocketevent(datanodefine);
			notify_observable(s);		
		}
	}
	clientclose(s);
	/* 通知订阅者 */
	setsocketevent(clientdiscon);
	notify_observable(s);
}

int CMSSocket::client_connect(std::string addr, int port)
{
	int s = make_skt();
	if (s == SOCKET_ERROR)
		return false;
	
	if (!connect_skt(s, addr, port))
	{
		return false;
	}

	std::thread thread(&CMSSocket::clientreceive_skt, this, s);
	thread.detach();

	return s;
}

int CMSSocket::sever_create(std::string addr, int port)
{
	int s = make_skt();
	if (s == SOCKET_ERROR)
		return false;

	if (!listen_skt(s, addr, port))
	{
		return false;
	}
	return s;
}

bool CMSSocket::get_recvbuf(int &socket, char **buffer)
{  
	if (_dataqueue.size() == 0)
	{
		return false;
	}

	do{
		std::lock_guard<std::mutex> lock(_mutex);
		std::shared_ptr<recvdata> pdata;
		pdata = _dataqueue.front();
		_dataqueue.pop();
		socket = pdata->socket;
		memcpy(*buffer, pdata->buffer, DATAPACKETSIZE);
	} while (0);

	return true;
};