/*******************************************************************************
* @file     MSSocket.h
* @brief    socket
* @author   linsn
* @date:    2021-9-16
******************************************************************************/
#pragma once
#include <string>
#include <list>
#include <vector>
#include <queue>
#include <thread>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include "SocketObservable.h"
#include "ThreadPool.h"

#define DATAPACKETSIZE 1024

enum socketevent
{
	clientrecv = 0,
		serverrecv,
		clientaccpet,
		clientdiscon,
		servercolse,
		datanodefine
};

#define WS_VERSION_CHOICE1 0x202/*MAKEWORD(2,2)*/
#define WS_VERSION_CHOICE2 0x101/*MAKEWORD(1,1)*/

class CMSSocket
{
public:
	CMSSocket();
	~CMSSocket();
private:
	/*Observable*/
	std::list<CSocketObservable*> _observerlist;
	socketevent _evnet;

	/*data*/
	std::mutex _mutexdata;
	struct recvdata{
		int socket;
		char buffer[DATAPACKETSIZE];
	};
	std::queue<std::shared_ptr<recvdata>> _dataqueue;

	/*threadpool*/
	ThreadPool *_pthreadpool;
public:
	/*Observable*/
	void attach_observable(CSocketObservable *observer);
	void dttach_observable(CSocketObservable* observer);
	void clear_observable(){ _observerlist.clear();};
	int  getsize_observable(){ return (int)_observerlist.size(); };
	socketevent getsocketevent(){ return _evnet; };
	void setsocketevent(socketevent event){ _evnet = event; };
	void notify_observable(int socket);

	/*clietsock*/
	std::vector<int> _clientsocklist;
	void addclientsock(int s);
	void delclientsock(int s);
	int  getclientsocksize(){ return (int)_clientsocklist.size(); };
private:
	/*basic*/
	int 	geterror_skt();
	int		init_skt();
	int		uninit_skt();
	int  	make_skt(); 
	void    close_skt(int s);
	void	setnoblocking_skt(int s);

	/*epoll*/
	void 	addepollfd_skt(int epollfd, int fd, bool oneshot);
	void	delepollfd_skt(int epollfd, int fd, bool oneshot);
	void 	resetonshot_skt(int epollfd, int fd);

	/*op*/
	bool    listen_skt(int s, std::string addr, int port);
	bool	connect_skt(int s, std::string addr, int port);

	/*thread*/
	void	serverselect_skt(int s);
	void	serverepoll_skt(int s);
	void    accpet_skt(int s);
	void    clientreceive_skt(int s);
	void    severreceive_skt(int s);
public:
	/*basic*/
	int		send_skt(int s, char *data, int len);
	int		receive_skt(int s, char *data, int len);
	void	clientclose(int s);
	void	severclose(int s);

	/*client*/
	int client_connect(std::string addr, int port);

	/*sever*/
	int sever_create(std::string addr, int port);

	/*get data*/
	bool get_recvbuf(int &socket, char **buffer);
};

