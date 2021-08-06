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
	case clientrevc: printf("user---socket:%d 客户端的信息\n", socket); break;
	case clientaccpet:printf("user---socket:%d 客户端连接\n", socket); break;
	case clientdiscon:printf("user---socket:%d 客户端断开连接\n", socket); break;
	case servercolse:printf("user---socket:%d 服务器关闭\n", socket); break;
	case datanodefine:printf("user---socket:%d 未定义数据\n", socket); break;
	case serverrecv:printf("user---socket:%d 服务器消息\n", socket);RecvDataProcess(socket);break;
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
		case COMMAND_SIGIN: /*注册*/
			{
				printf("%s---收到注册申请！\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---昵称：%s\n", userinfo.nickname);
				printf("data---账户：%d\n", userinfo.userid);
				printf("data---密码：%s\n", userinfo.password);
				printf("data---签名：%s\n", userinfo.userdescription);

				CommandSigin(socket);
			}
			break;
		case COMMAND_SIGOUT: /*注销*/
			{
				printf("%s---收到注销申请！\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---账户：%d\n", userinfo.userid);
				printf("data---密码：%s\n", userinfo.password);
			}
			break;
		case COMMAND_LOGIN:/*登入*/
			{
				printf("%s---收到登录申请！\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---账户：%d\n", userinfo.userid);
				printf("data---密码：%s\n", userinfo.password);

				CommandLogin(socket);
			}
			break;
		case COMMAND_LOGOUT:/*登出*/
			{
				printf("%s---收到退出登录申请！\n", GetCurTime().c_str());
				USERINFO userinfo;
				memcpy(&userinfo, transinfo.data, sizeof(userinfo));
				printf("data---账户：%d\n", userinfo.userid);
				printf("data---密码：%s\n", userinfo.password);
			}
			break;
		case COMMAND_ADDFRIEND:/*添加好友*/
			{
				printf("%s---收到添加好友申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---申请人：%d\n", chatinfo.useridfrom);
				printf("data---要添加的好友：%d\n", chatinfo.useridto);
				printf("data---打招呼：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_DELFRIEND:/*删除好友*/
			{
				printf("%s---收到删除好友申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---申请人：%d\n", chatinfo.useridfrom);
				printf("data---要删除的好友：%d\n", chatinfo.useridto);
				printf("data---寄语：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_ADDGROUND:/*加入群聊*/
			{
				printf("%s---收到加入群聊申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---申请人：%d\n", chatinfo.useridfrom);
				printf("data---要加入的群：%d\n", chatinfo.useridto);
				printf("data---打招呼：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_DELGROUND:/*退出群聊*/
			{
				printf("%s---收到退出群聊申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---申请人：%d\n", chatinfo.useridfrom);
				printf("data---要退出的群：%d\n", chatinfo.useridto);
				printf("data---寄语：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_SINGLECHAT:/*单聊*/
			{
				printf("%s---收到单聊信息！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---发送人：%d\n", chatinfo.useridfrom);
				printf("data---接收人：%d\n", chatinfo.useridto);
				printf("data---信息：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_GROUPCHAT:/*群聊*/
			{
				printf("%s---收到	群聊信息！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, transinfo.data, sizeof(chatinfo));
				printf("data---发送人：%d\n", chatinfo.useridfrom);
				printf("data---接收群：%d\n", chatinfo.useridto);
				printf("data---信息：%s\n", chatinfo.info);
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
		printf("log---注册申请回复发送失败\n");
	}
	else
	{
		printf("log---注册申请回复成功\n");
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
		printf("log---登录申请回复发送失败\n");
	}
	else
	{
		printf("log---登录申请回复成功\n");
	}
	delete[] buff;
}