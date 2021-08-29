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
#define _NonBlocking		1   /*非堵塞I/O*/
#define _IOMODEL_  _NonBlocking

std::mutex g_mutex;

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

#if _IOMODEL_ == _NonBlocking
	/*设置非堵塞模式*/
#ifdef _WIN32
	unsigned long ul = 1;
	ioctlsocket(s, FIONBIO, (unsigned long *)&ul);
#else
	int flags = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
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

	if (listen(s, 5) == SOCKET_ERROR)
	{
		printf("socket---listen error,code:%d\n", geterror_skt());
		return false;
	}

#if _IOMODEL_ == _NonBlocking
	std::thread selectthread(&CMSSocket::server_skt, this, s);
	selectthread.detach();
#elif _IOMODEL_ == _Blocking
	std::thread thread(&CMSSocket::accpet_skt, this, s);
	thread.detach();
#endif
	return true;
}

void CMSSocket::server_skt(int s)
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

				/* 通知订阅者 */
				setsocketevent(clientaccpet);
				notify_observable(clientfd);

				continue;
				//printf("ip=%s\n", inet_ntoa(clientaddr.sin_addr));
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
		for (int i = 0; i<clientsize; i++)
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
					
					clientclose(_clientsocklist[i]);
					//break;
				}
				else
				{
					recvdata data;
					memcpy(&data, rbuffer, DATAPACKETSIZE);
					std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
					_dataqueue.push(recvda);

					/* 通知订阅者 */
					if (datalen == DATAPACKETSIZE)
						setsocketevent(serverrecv);
					else
						setsocketevent(datanodefine);
					notify_observable(_clientsocklist[i]);
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

bool CMSSocket::get_recvbuf(char **buffer)
{  
	if (_dataqueue.size() == 0)
	{
		return false;
	}

	std::shared_ptr<recvdata> pdata;
	pdata = _dataqueue.front();
	_dataqueue.pop();

	memcpy(*buffer, pdata->buffer, DATAPACKETSIZE);

	return true;
};