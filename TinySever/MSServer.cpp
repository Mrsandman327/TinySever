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
	case clientrevc: printf("user---socket:%d �ͻ��˵���Ϣ\n", socket); break;
	case clientaccpet:printf("user---socket:%d �ͻ�������\n", socket); break;
	case clientdiscon:printf("user---socket:%d �ͻ��˶Ͽ�����\n", socket); break;
	case servercolse:printf("user---socket:%d �������ر�\n", socket); break;
	case datanodefine:printf("user---socket:%d δ��������\n", socket); break;
	case serverrecv:printf("user---socket:%d ��������Ϣ\n", socket);RecvDataProcess();break;
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
		printf("����:%s\n", transinfo.filedata);

	}
	delete[] buffer;
}