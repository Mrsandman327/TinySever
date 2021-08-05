#include "MSServer.h"


CMSServer::CMSServer(CMSSocket *sub)
{
	this->_socket = sub;
}


CMSServer::~CMSServer()
{
}

void CMSServer::Update(int socket)
{
	socketevent event = _socket->getsocketevent();
	switch (event)
	{
	case clientrevc: printf("user---socket:%d 客户端的信息\n", socket); break;
	case clientaccpet:printf("user---socket:%d 客户端连接\n", socket); break;
	case clientdiscon:printf("user---socket:%d 客户端断开连接\n", socket); break;
	case servercolse:printf("user---socket:%d 服务器关闭\n", socket); break;
	case datanodefine:printf("user---socket:%d 未定义数据\n", socket); break;
	case serverrecv:printf("user---socket:%d 服务器消息\n", socket);RecvDataProcess();break;
	default:
		break;
	}
}



void CMSServer::RecvDataProcess()
{
	char *buffer = new char[DATAPACKETSIZE];
	if (_socket->get_recvbuf(&buffer))
	{
		TransInfo transinfo;
		memcpy(&transinfo, buffer, sizeof(transinfo));
		printf("接收:%s\n", transinfo.filedata);

	}
	delete[] buffer;
}