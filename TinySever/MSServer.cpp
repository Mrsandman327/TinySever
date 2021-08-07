#include "MSServer.h"
#include <ctime>

CMSServer::CMSServer(CMSSocket *sub)
{
	this->_socket = sub;
	ReadUserInfoJson();
}

CMSServer::~CMSServer()
{
	_oJsonuserinfo.Clear();
	_mapUserOnline.clear();
}

void CMSServer::ReadUserInfoJson()
{
	std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(USERJSONFILE, std::ios::in);
	std::string strjson;
	std::string strtemp;
	while (*content >> strtemp) {
		strjson += strtemp;
	}
	_oJsonuserinfo.Parse(strjson);

	printf("%s\n", _oJsonuserinfo.ToFormattedString().c_str());
}

void CMSServer::SaveUserInfoJson()
{
	std::string strjson = _oJsonuserinfo.ToFormattedString();
	std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(USERJSONFILE, std::ios::out);
	*content << strjson;

	printf("%s\n", strjson.c_str());
}

bool CMSServer::MapRomoveByValue(int value)
{
	int res = false;
	for (auto iter = _mapUserOnline.begin(); iter != _mapUserOnline.end(); iter++)
	{
		if (iter->second == value)
		{
			_mapUserOnline.erase(iter->first);
			res = true;
			break;
		}
	}
	return res;
}

bool CMSServer::MapRomoveByKey(unsigned int key)
{
	_mapUserOnline.erase(key);

	return true;
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
	case clientdiscon:printf("user---socket:%d �ͻ��˶Ͽ�����\n", socket); MapRomoveByValue(socket);break;
	case servercolse:printf("user---socket:%d �������ر�\n", socket);_oJsonuserinfo.Clear();_mapUserOnline.clear(); break;
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
		std::function<void(int,DATA_PACK*)> CommandReturn;

		DATA_PACK datapack;
		memcpy(&datapack, buffer, sizeof(datapack));

		switch (COMMANDTYPE(datapack.commandtype))
		{
		case COMMAND_SIGIN: /*ע��*/
			{
				printf("%s---�յ�ע�����룡\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSiginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_SIGOUT: /*ע��*/
			{
				printf("%s---�յ�ע�����룡\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSigoutReturn, this, std::placeholders::_1, std::placeholders::_2);	
			}
			break;
		case COMMAND_LOGIN:/*����*/
			{
				printf("%s---�յ���¼���룡\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandLoginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_LOGOUT:/*�ǳ�*/
			{
				printf("%s---�յ��˳���¼���룡\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandLogoutReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDFRIEND:/*��Ӻ���*/
			{
				printf("%s---�յ���Ӻ������룡\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandAddFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_DELFRIEND:/*ɾ������*/
			{
				printf("%s---�յ�ɾ���������룡\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandDelFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDGROUND:/*����Ⱥ��*/
			{
				printf("%s---�յ�����Ⱥ�����룡\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---Ҫ�����Ⱥ��%d\n", chatinfo.useridto);
				printf("data---���к���%s\n", chatinfo.info);
			}
			break;
		case COMMAND_DELGROUND:/*�˳�Ⱥ��*/
			{
				printf("%s---�յ��˳�Ⱥ�����룡\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---Ҫ�˳���Ⱥ��%d\n", chatinfo.useridto);
				printf("data---���%s\n", chatinfo.info);
			}
			break;
		case COMMAND_SINGLECHAT:/*����*/
			{
				printf("%s---�յ�������Ϣ��\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSingleChatReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_GROUPCHAT:/*Ⱥ��*/
			{
				printf("%s---�յ�Ⱥ����Ϣ��\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---����Ⱥ��%d\n", chatinfo.useridto);
				printf("data---��Ϣ��%s\n", chatinfo.info);
			}
			break;
		case COMMAND_FRIENDINFO:/*Ⱥ��*/
			{
				printf("%s---�յ���ȡ������Ϣ���룡\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandFriendInfoReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		default:
			break;
		}
		CommandReturn(socket, &datapack);
	}
	delete[] buffer;
}

void CMSServer::SendDataPackReturn(int socket, COMMANDTYPE type, RESULT result, void* data, int size)
{
	DATA_PACK datapack;
	memset(&datapack, 0, sizeof(DATA_PACK));

	RESULTINFO_RETURN  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO_RETURN));
	resultinfo.result = result;
	memcpy(resultinfo.resultinfo, (char*)data, size);

	/*������*/
	datapack.commandtype = type;
	datapack.datatype = RESULT_RETURN;
	memcpy(datapack.data, &resultinfo, sizeof(RESULTINFO_RETURN));

	/*���ͽ��*/
	char *buff = new char[sizeof(DATA_PACK)];
	memset(buff, 0, sizeof(DATA_PACK));
	memcpy(buff, &datapack, sizeof(DATA_PACK));
	if (0 != _socket->send_skt(socket, (char*)buff, sizeof(DATA_PACK)))
	{
		printf("log---����%d�ظ�ʧ��\n", type);
	}
	else
	{
		printf("log---����%d�ظ��ɹ�\n",type);
	}
	delete[] buff;
}

bool CMSServer::SendDataPack(int socket, DATA_PACK *datapack)
{
	bool result = false;
	/*���ͽ��*/
	char *buff = new char[sizeof(DATA_PACK)];
	memset(buff, 0, sizeof(DATA_PACK));
	memcpy(buff, datapack, sizeof(DATA_PACK));
	if (0 == _socket->send_skt(socket, (char*)buff, sizeof(DATA_PACK)))
	{
		result = true;
	}
	delete[] buff;

	return result;
}

void CMSServer::CommandSiginReturn(int socket, DATA_PACK *datapack)
{
	SIGIN_INFO sigininfo;
	memcpy(&sigininfo, datapack->data, sizeof(SIGIN_INFO));
	printf("data---�ǳƣ�%s\n", sigininfo.nickname);
	printf("data---�˻���%d\n", sigininfo.userid);
	printf("data---���룺%s\n", sigininfo.password);
	printf("data---ǩ����%s\n", sigininfo.userdescription);

	/*У��ע����Ϣ*/
	bool result = true;
	std::string errorinfo = "";
	if (sigininfo.userid < 99999 || sigininfo.userid >999999999)
	{
		result = false;
		errorinfo = "�˺Ų����Ϲ涨";
		goto LLL;
	}
	
	/*д��ע����Ϣ*/
	{
		neb::CJsonObject oJson;
		oJson.Add("nickname", sigininfo.nickname);
		oJson.Add("userid", sigininfo.userid);
		oJson.Add("password", sigininfo.password);
		oJson.Add("description", sigininfo.userdescription);
		oJson.AddEmptySubArray("friend");
		oJson.AddEmptySubArray("group");
		_oJsonuserinfo["userinfo"].Add(oJson);

		SaveUserInfoJson();
	}
	
LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;;
	if (result)
	{
		data = nullptr;
		printf("log---ע��ɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---ע��ʧ��ʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandSigoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);
	
	/*У���¼��Ϣ*/
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "���˻���δ��½";
		goto LLL;
	}
	
	int size = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		std::string password;
		oJson.Get("userid", userid);
		oJson.Get("password", password);

		if (userinfo.userid == userid)
		{
			if (stpassword == password)
			{
				_oJsonuserinfo["userinfo"].Delete(i);
				SaveUserInfoJson();
				result = true;
				break;
			}
			else
			{
				errorinfo = "�������";
			}
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "���û���δע��";
	}
	
LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		printf("log---ע���ɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---ע��ʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLoginReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);
	
	/*У���¼��Ϣ*/	
	USERINFO_RETURN userinfo_return;
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "���˻��Ѿ���½";
		goto LLL;
	}
	
	int size = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		std::string password;
		oJson.Get("userid", userid);
		oJson.Get("password", password);

		if (userinfo.userid == userid)
		{
			if (stpassword == password)
			{
				_mapUserOnline.insert(std::pair<unsigned int,int>(userid, socket));
				/*��ȡ������û���Ϣ*/
				std::string nickname, description;
				oJson.Get("nickname", nickname);
				oJson.Get("description", description);
				strcpy(userinfo_return.nickname, nickname.c_str());
				strcpy(userinfo_return.userdescription, description.c_str());
				
				result = true;
				break;
			}
			else
			{
				errorinfo = "�������";
				break;
			}
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "���û���δע��";
	}

LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &userinfo_return;
		datasize = sizeof(USERINFO_RETURN);
		printf("log---��¼�ɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---��¼ʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLogoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);


	/*У���¼��Ϣ*/
	bool result = false;
	std::string errorinfo = "";
	if (_mapUserOnline.count(userinfo.userid))
	{
		MapRomoveByKey(userinfo.userid);
		result = true;
	}
	else
	{
		errorinfo = "���û�δ��¼";
	}

LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		printf("log---�˳���¼�ɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---�˳���¼ʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
	
}

void CMSServer::CommandAddFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friend_info;
	memcpy(&friend_info, datapack->data, sizeof(FRIEND_INFO));
	printf("data---�����ˣ�%d\n", friend_info.userid);
	printf("data---Ҫ��ӵĺ��ѣ�%d\n", friend_info.friendid);
	printf("data---���к���%s\n", friend_info.info);

	/*У�������Ϣ*/
	FRIENDINFO_RETURN friendinfo;
	bool result = false;
	std::string errorinfo = "";

	if (friend_info.userid == friend_info.friendid)
	{
		errorinfo = "�޷�����Լ�";
		goto LLL;
	}

	/*������ӵĺ����Ƿ�ע��*/
	int size = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (friend_info.friendid == userid)
		{
			/*��ȡ��ӵĺ�����Ϣ*/
			std::string nickname, description;
			oJson.Get("nickname", nickname);
			oJson.Get("description", description);
			strcpy(friendinfo.nickname, nickname.c_str());
			strcpy(friendinfo.userdescription, description.c_str());

			result = true;
			break;
		}
	}
		
	if (!result)
	{
		errorinfo = "���û���δע��";
		goto LLL;
	}

	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (friend_info.userid == userid)
		{
			/*�������Ƿ��Ѿ�����*/
			int sz = oJson["friend"].GetArraySize();
			for (size_t j = 0; j < sz; j++)
			{
				unsigned int friendid;
				oJson["friend"].Get(j, friendid);
				if (friend_info.friendid == friendid)
				{
					result = false;
					break;
				}
			}
			if (result)
			{
				oJson["friend"].Add(friend_info.friendid);
				_oJsonuserinfo["userinfo"].Replace(i, oJson);
				SaveUserInfoJson();
				result = true;
				break;
			}
			else
			{
				errorinfo = "��������ӹ�";
			}
			break;
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "��ǰ�û�δע��";
	}

LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &friendinfo;
		datasize = sizeof(FRIENDINFO_RETURN);
		printf("log---��¼�ɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---��¼ʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandDelFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friendinfo;
	memcpy(&friendinfo, datapack->data, sizeof(FRIEND_INFO));
	printf("data---�����ˣ�%d\n", friendinfo.userid);
	printf("data---Ҫ��ӵĺ��ѣ�%d\n", friendinfo.friendid);
	printf("data---���%s\n", friendinfo.info);

	/*У�������Ϣ*/
	bool result = false;
	std::string errorinfo = "";

	if (!_mapUserOnline.count(friendinfo.userid))
	{
		errorinfo = "�û�δ��¼";
		goto LLL;
	}

	if (friendinfo.userid == friendinfo.friendid)
	{
		errorinfo = "�޷�ɾ���Լ�";
		goto LLL;
	}

	int size = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (friendinfo.userid == userid)
		{
			int sz = oJson["friend"].GetArraySize();
			for (size_t j = 0; j < sz; j++)
			{
				unsigned int friendid;
				oJson["friend"].Get(j, friendid);
				if (friendinfo.friendid == friendid)
				{
					oJson["friend"].Delete(j);
					_oJsonuserinfo["userinfo"].Replace(i, oJson);
					SaveUserInfoJson();
					result = true;
					break;
				}	
			}
			if (!result)
			{
				errorinfo = "��û�иú���";
			}
			break;
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "��ǰ�û���δע��";
	}

LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		printf("log---ɾ�����ѳɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---ɾ������ʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandSingleChatReturn(int socket, DATA_PACK *datapack)
{
	CHATINFO chatinfo;
	memcpy(&chatinfo, datapack->data, sizeof(chatinfo));
	printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
	printf("data---�����ˣ�%d\n", chatinfo.useridto);
	printf("data---��Ϣ��%s\n", chatinfo.info);

	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(chatinfo.useridto))
	{
		errorinfo = "����δ��¼";
		goto LLL;
	}

	int sock =  _mapUserOnline[chatinfo.useridto];

	if (!SendDataPack(sock, datapack))
		errorinfo = "��Ϣ����ʧ��";
	else
		result = true;

LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		printf("log---����������Ϣ�ɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---����������Ϣʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandFriendInfoReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);

	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "�û�δ��¼";
		goto LLL;
	}

	ALLRIENDINFO_RETURN friendinfo;
	int size = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);

		unsigned int userid;
		oJson.Get("userid", userid);
		if (userinfo.userid == userid)
		{
			friendinfo.friendsize = oJson["friend"].GetArraySize();
			for (size_t j = 0; j < friendinfo.friendsize; j++)
			{
				oJson["friend"].Get(j, friendinfo.friendid[j]);
			}
			result = true;
			break;
		}
	}

	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		for (size_t j = 0; j < friendinfo.friendsize; j++)
		{
			if (userid == friendinfo.friendid[j])
			{
				std::string nickname, description;
				oJson.Get("nickname", nickname);
				oJson.Get("description", description);
				strcpy(friendinfo.nickname[j], nickname.c_str());
				strcpy(friendinfo.userdescription[j], description.c_str());
			}
		}
	}

LLL:
	/*������*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &friendinfo;
		datasize = sizeof(ALLRIENDINFO_RETURN);
		printf("log---��ȡ���ѳɹ�\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---��ȡ����ʧ�� ������Ϣ��%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}