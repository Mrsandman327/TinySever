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

	printf(_oJsonuserinfo.ToFormattedString().c_str());
}

void CMSServer::SaveUserInfoJson()
{
	std::string strjson = _oJsonuserinfo.ToFormattedString();
	std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(USERJSONFILE, std::ios::out);
	*content << strjson;

	printf(strjson.c_str());
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
	case clientdiscon:printf("user---socket:%d �ͻ��˶Ͽ�����\n", socket); 
	if (_mapUserOnline.count(socket))
	{
		_mapUserOnline.erase(socket);
	}
	break;
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
		std::function<void(int,DATAPACK*)> CommandReturn;

		DATAPACK datapack;
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
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---�����ˣ�%d\n", chatinfo.useridto);
				printf("data---��Ϣ��%s\n", chatinfo.info);
			}
			break;
		case COMMAND_GROUPCHAT:/*Ⱥ��*/
			{
				printf("%s---�յ�	Ⱥ����Ϣ��\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
				printf("data---����Ⱥ��%d\n", chatinfo.useridto);
				printf("data---��Ϣ��%s\n", chatinfo.info);
			}
			break;
		default:
			break;
		}
		CommandReturn(socket, &datapack);
	}
	delete[] buffer;
}

bool CMSServer::SendDataPackReturn(int socket, DATAPACK *datapack)
{
	bool result = false;
	/*���ͽ��*/
	char *buff = new char[sizeof(DATAPACK)];
	memset(buff, 0, sizeof(DATAPACK));
	memcpy(buff, datapack, sizeof(DATAPACK));
	if (0 != _socket->send_skt(socket, (char*)buff, sizeof(DATAPACK)))
	{
		result = false;
	}
	else
	{
		result = true;
	}
	delete[] buff;

	return result;
}

void CMSServer::CommandSiginReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---�ǳƣ�%s\n", userinfo.nickname);
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);
	printf("data---ǩ����%s\n", userinfo.userdescription);

	/*У��ע����Ϣ*/
	bool result = true;
	std::string errorinfo = "";
	if (userinfo.userid < 99999 || userinfo.userid >999999999)
	{
		result = false;
		errorinfo = "�˺Ų����Ϲ涨";
	}
	else
	{
		/*д��ע����Ϣ*/
		neb::CJsonObject oJson;
		oJson.Add("nickname", userinfo.nickname);
		oJson.Add("userid", userinfo.userid);
		oJson.Add("password", userinfo.password);
		oJson.Add("description", userinfo.userdescription);
		oJson.AddEmptySubArray("friend");
		oJson.AddEmptySubArray("group");
		_oJsonuserinfo["userinfo"].Add(oJson);

		SaveUserInfoJson();
	}

	/*������*/
	datapack->commandtype = COMMAND_SIGIN;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---ע��ɹ�\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*���ͽ��*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---ע������ظ�����ʧ��\n");
	else
		printf("log---ע������ظ��ɹ�\n");
}

void CMSServer::CommandSigoutReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);

	/*У���¼��Ϣ*/
	USERINFOALL userinfoall;
	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(socket))
	{
		errorinfo = "���˻���δ��½";
	}
	else
	{
		std::string stpassword = userinfo.password;
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
	}
	/*������*/
	datapack->commandtype = COMMAND_SIGOUT;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		memcpy(resultinfo.resultinfo, &userinfoall, sizeof(USERINFOALL));
		resultinfo.result = OK;

		printf("log---ע���ɹ�\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*���ͽ��*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---ע������ظ�����ʧ��\n");
	else
		printf("log---ע������ظ��ɹ�\n");
}

void CMSServer::CommandLoginReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);
	
	/*У���¼��Ϣ*/	
	USERINFOALL userinfoall;
	bool result = false;
	std::string errorinfo = "";
	if (_mapUserOnline.count(socket))
	{
		errorinfo = "���˻��Ѿ���½";
	}
	else
	{
		std::string stpassword = userinfo.password;
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
					_mapUserOnline.insert(std::pair<int, unsigned int>(socket, userid));
					/*��ȡ������û�������Ϣ*/
					std::string nickname, description;
					oJson.Get("nickname", nickname);
					oJson.Get("description", description);
					strcpy(userinfoall.nickname, nickname.c_str());
					userinfoall.userid = userinfo.userid;
					strcpy(userinfoall.password, userinfo.password);
					strcpy(userinfoall.userdescription, description.c_str());
					userinfoall.friendsize = oJson["friend"].GetArraySize();
					userinfoall.groupsize = oJson["group"].GetArraySize();
					for (int i = 0; i < userinfoall.friendsize; ++i)
					{
						unsigned int friendid;
						oJson["friend"].Get(i, friendid);
						userinfoall.friendlist[i] = friendid;
					}

					for (int i = 0; i < userinfoall.groupsize; ++i)
					{
						unsigned int groupid;
						oJson["group"].Get(i, groupid);
						userinfoall.grouplist[i] = groupid;
					}
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
	}

	/*������*/
	datapack->commandtype = COMMAND_LOGIN;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		memcpy(resultinfo.resultinfo, &userinfoall, sizeof(USERINFOALL));
		resultinfo.result = OK;

		printf("log---��¼�ɹ�\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---%s\n",resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*���ͽ��*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---��¼����ظ�����ʧ��\n");
	else
		printf("log---��¼����ظ��ɹ�\n");
}

void CMSServer::CommandLogoutReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---�˻���%d\n", userinfo.userid);
	printf("data---���룺%s\n", userinfo.password);


	/*У���¼��Ϣ*/
	bool result = false;
	std::string errorinfo = "";
	if(_mapUserOnline.count(socket))
	{
		_mapUserOnline.erase(socket);
		result = true;
	}
	else
	{
		errorinfo = "���û�δ��¼";
	}

	/*������*/
	datapack->commandtype = COMMAND_LOGOUT;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---�˳���¼�ɹ�\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*���ͽ��*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---�˳���¼����ظ�����ʧ��\n");
	else
		printf("log---�˳���¼����ظ��ɹ�\n");
	
}

void CMSServer::CommandAddFriendReturn(int socket, DATAPACK *datapack)
{
	CHATINFO chatinfo;
	memcpy(&chatinfo, datapack->data, sizeof(chatinfo));
	printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
	printf("data---Ҫ��ӵĺ��ѣ�%d\n", chatinfo.useridto);
	printf("data---���к���%s\n", chatinfo.info);

	/*У�������Ϣ*/
	bool result = false;
	std::string errorinfo = "";

	if (chatinfo.useridfrom == chatinfo.useridto)
	{
		errorinfo = "�޷�����Լ�";
	}
	else
	{
		/*������ӵĺ����Ƿ�ע��*/
		int size = _oJsonuserinfo["userinfo"].GetArraySize();
		for (int i = 0; i < size; ++i)
		{
			neb::CJsonObject oJson;
			_oJsonuserinfo["userinfo"].Get(i, oJson);
			unsigned int userid;
			oJson.Get("userid", userid);
			if (chatinfo.useridto == userid &&
				chatinfo.useridfrom != userid)
			{
				result = true;
				break;
			}
		}
		if (result)
		{
			result = false;
			for (int i = 0; i < size; ++i)
			{
				neb::CJsonObject oJson;
				_oJsonuserinfo["userinfo"].Get(i, oJson);
				unsigned int userid;
				oJson.Get("userid", userid);
				if (chatinfo.useridfrom == userid)
				{
					/*�������Ƿ��Ѿ�����*/
					int sz = oJson["friend"].GetArraySize();
					for (size_t j = 0; j < sz; j++)
					{
						unsigned int friendid;
						oJson["friend"].Get(j, friendid);
						if (chatinfo.useridto == friendid)
						{
							result = false;
							break;
						}
					}
					if (result)
					{
						oJson["friend"].Add(chatinfo.useridto);
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
		}
		else
		{
			errorinfo = "���û���δע��";
		}
	}

	/*������*/
	datapack->commandtype = COMMAND_ADDFRIEND;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---��Ӻ��ѳɹ�\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*���ͽ��*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---��Ӻ�������ظ�����ʧ��\n");
	else
		printf("log---��Ӻ�������ظ��ɹ�\n");
}

void CMSServer::CommandDelFriendReturn(int socket, DATAPACK *datapack)
{
	CHATINFO chatinfo;
	memcpy(&chatinfo, datapack->data, sizeof(chatinfo));
	printf("data---�����ˣ�%d\n", chatinfo.useridfrom);
	printf("data---Ҫ��ӵĺ��ѣ�%d\n", chatinfo.useridto);
	printf("data---���%s\n", chatinfo.info);

	/*У�������Ϣ*/
	bool result = false;
	std::string errorinfo = "";

	if (chatinfo.useridfrom == chatinfo.useridto)
	{
		errorinfo = "�޷�ɾ���Լ�";
	}
	else
	{
		int size = _oJsonuserinfo["userinfo"].GetArraySize();
		for (int i = 0; i < size; ++i)
		{
			neb::CJsonObject oJson;
			_oJsonuserinfo["userinfo"].Get(i, oJson);
			unsigned int userid;
			oJson.Get("userid", userid);
			if (chatinfo.useridfrom == userid)
			{
				int sz = oJson["friend"].GetArraySize();
				for (size_t j = 0; j < sz; j++)
				{
					unsigned int friendid;
					oJson["friend"].Get(j, friendid);
					if (chatinfo.useridto == friendid)
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
	}

	/*������*/
	datapack->commandtype = COMMAND_DELFRIEND;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---ɾ�����ѳɹ�\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*���ͽ��*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---ɾ����������ظ�����ʧ��\n");
	else
		printf("log---ɾ����������ظ��ɹ�\n");
}