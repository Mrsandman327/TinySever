#include "MSServer.h"
#include <ctime>

CMSServer::CMSServer(CMSSocket *sub)
{
	this->_socket = sub;
}


CMSServer::~CMSServer()
{
}

std::string CMSServer::GetCurTime()
{
	time_t now = time(0);
	tm *ltm = localtime(&now);

	char timestr[64];
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", ltm);
	return timestr;
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
	case serverrecv:printf("user---socket:%d ��������Ϣ\n", socket);RecvDataProcess(socket);break;
	default:
		break;
	}
}

void CMSServer::RecvDataProcess(int socket)
{
	char *buffer = new char[DATAPACKETSIZE];
	if (_socket->get_recvbuf(&buffer))
	{
		DATAPACK transinfo;
		memcpy(&transinfo, buffer, sizeof(transinfo));

		switch (COMMANDTYPE(transinfo.commandtype))
		{
		case COMMAND_SIGIN: /*ע��*/
			{
				printf("%s---�յ�ע�����룡\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---�ǳƣ�%s\n", userinfo.nickname);
				printf("data---�˻���%d\n", userinfo.userid);
				printf("data---���룺%s\n", userinfo.password);
				printf("data---ǩ����%s\n", userinfo.userdescription);

				CommandSigin(socket);
			}
			break;
		case COMMAND_SIGOUT: /*ע��*/
			{
				printf("%s---�յ�ע�����룡\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---�˻���%d\n", userinfo.userid);
				printf("data---���룺%s\n", userinfo.password);
			}
			break;
		case COMMAND_LOGIN:/*����*/
			{
				printf("%s---�յ���¼���룡\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---�˻���%d\n", userinfo.userid);
				printf("data---���룺%s\n", userinfo.password);

				CommandLogin(socket);
			}
			break;
		case COMMAND_LOGOUT:/*�ǳ�*/
			{
				printf("%s---�յ��˳���¼���룡\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---�˻���%d\n", userinfo.userid);
				printf("data---���룺%s\n", userinfo.password);
			}
			break;
		case COMMAND_ADDFRIEND:/*��Ӻ���*/
			{
				printf("%s---�յ���Ӻ������룡\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---Ҫ��ӵĺ��ѣ�%d\n", chatinfo.useridto);
				printf("data---���к���%s\n", chatinfo.info);
			}
			break;
		case COMMAND_DELFRIEND:/*ɾ������*/
			{
				printf("%s---�յ�ɾ���������룡\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---Ҫɾ���ĺ��ѣ�%d\n", chatinfo.useridto);
				printf("data---���%s\n", chatinfo.info);
			}
			break;
		case COMMAND_ADDGROUND:/*����Ⱥ��*/
			{
				printf("%s---�յ�����Ⱥ�����룡\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---Ҫ�����Ⱥ��%d\n", chatinfo.useridto);
				printf("data---���к���%s\n", chatinfo.info);
			}
			break;
		case COMMAND_DELGROUND:/*�˳�Ⱥ��*/
			{
				printf("%s---�յ��˳�Ⱥ�����룡\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---Ҫ�˳���Ⱥ��%d\n", chatinfo.useridto);
				printf("data---���%s\n", chatinfo.info);
			}
			break;
		case COMMAND_SINGLECHAT:/*����*/
			{
				printf("%s---�յ�������Ϣ��\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---�����ˣ�%d\n", chatinfo.useridto);
				printf("data---��Ϣ��%s\n", chatinfo.info);
			}
			break;
		case COMMAND_GROUPCHAT:/*Ⱥ��*/
			{
				printf("%s---�յ�	Ⱥ����Ϣ��\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---����Ⱥ��%d\n", chatinfo.useridto);
				printf("data---��Ϣ��%s\n", chatinfo.info);
			}
			break;
		default:
			break;
		}
	}
	delete[] buffer;
}


void CMSServer::CommandSigin(int socket)
{
	DATAPACK datapack;
	datapack.commandtype = COMMAND_SIGIN;
	datapack.datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	resultinfo.result = OK;

	memcpy(datapack.data, &resultinfo, sizeof(RESULTINFO));

	char *buff = new char[sizeof(datapack)];
	memset(buff, 0, sizeof(datapack));
	memcpy(buff, &datapack, sizeof(datapack));
	if (1 == _socket->send_skt(socket, (char*)buff, sizeof(datapack)))
	{
		printf("log---ע������ظ�����ʧ��\n");
	}
	else
	{
		printf("log---ע������ظ��ɹ�\n");
	}
	delete[] buff;
}


void CMSServer::CommandLogin(int socket)
{
	DATAPACK datapack;
	datapack.commandtype = COMMAND_LOGIN;
	datapack.datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	resultinfo.result = OK;

	memcpy(datapack.data, &resultinfo, sizeof(RESULTINFO));

	char *buff = new char[sizeof(datapack)];
	memset(buff, 0, sizeof(datapack));
	memcpy(buff, &datapack, sizeof(datapack));
	if (1 == _socket->send_skt(socket, (char*)buff, sizeof(datapack)))
	{
		printf("log---��¼����ظ�����ʧ��\n");
	}
	else
	{
		printf("log---��¼����ظ��ɹ�\n");
	}
	delete[] buff;
}