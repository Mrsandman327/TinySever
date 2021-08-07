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
	case clientrevc: printf("user---socket:%d 客户端的信息\n", socket); break;
	case clientaccpet:printf("user---socket:%d 客户端连接\n", socket); break;
	case clientdiscon:printf("user---socket:%d 客户端断开连接\n", socket); MapRomoveByValue(socket);break;
	case servercolse:printf("user---socket:%d 服务器关闭\n", socket);_oJsonuserinfo.Clear();_mapUserOnline.clear(); break;
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
		std::function<void(int,DATAPACK*)> CommandReturn;

		DATAPACK datapack;
		memcpy(&datapack, buffer, sizeof(datapack));

		switch (COMMANDTYPE(datapack.commandtype))
		{
		case COMMAND_SIGIN: /*注册*/
			{
				printf("%s---收到注册申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSiginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_SIGOUT: /*注销*/
			{
				printf("%s---收到注销申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSigoutReturn, this, std::placeholders::_1, std::placeholders::_2);	
			}
			break;
		case COMMAND_LOGIN:/*登入*/
			{
				printf("%s---收到登录申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandLoginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_LOGOUT:/*登出*/
			{
				printf("%s---收到退出登录申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandLogoutReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDFRIEND:/*添加好友*/
			{
				printf("%s---收到添加好友申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandAddFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_DELFRIEND:/*删除好友*/
			{
				printf("%s---收到删除好友申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandDelFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDGROUND:/*加入群聊*/
			{
				printf("%s---收到加入群聊申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---申请人：%d\n", chatinfo.useridfrom);
				printf("data---要加入的群：%d\n", chatinfo.useridto);
				printf("data---打招呼：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_DELGROUND:/*退出群聊*/
			{
				printf("%s---收到退出群聊申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---申请人：%d\n", chatinfo.useridfrom);
				printf("data---要退出的群：%d\n", chatinfo.useridto);
				printf("data---寄语：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_SINGLECHAT:/*单聊*/
			{
				printf("%s---收到单聊信息！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSingleChatReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_GROUPCHAT:/*群聊*/
			{
				printf("%s---收到群聊信息！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				printf("data---发送人：%d\n", chatinfo.useridfrom);
				printf("data---接收群：%d\n", chatinfo.useridto);
				printf("data---信息：%s\n", chatinfo.info);
			}
			break;
		case COMMAND_FRIENDINFO:/*群聊*/
			{
				printf("%s---收到获取好友信息申请！\n", GetCurTime().c_str());
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

bool CMSServer::SendDataPackReturn(int socket, DATAPACK *datapack)
{
	bool result = false;
	/*发送结果*/
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
	printf("data---昵称：%s\n", userinfo.nickname);
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);
	printf("data---签名：%s\n", userinfo.userdescription);

	/*校验注册信息*/
	bool result = true;
	std::string errorinfo = "";
	if (userinfo.userid < 99999 || userinfo.userid >999999999)
	{
		result = false;
		errorinfo = "账号不符合规定";
		goto LLL;
	}
	
	/*写入注册信息*/
	{
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
	

LLL:
	/*打包结果*/
	datapack->commandtype = COMMAND_SIGIN;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---注册成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---注册失败失败 错误信息：%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---注册申请回复发送失败\n");
	else
		printf("log---注册申请回复发送成功\n");
}

void CMSServer::CommandSigoutReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);
	
	/*校验登录信息*/
	USERINFOALL userinfoall;	
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "该账户尚未登陆";
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
				errorinfo = "密码错误";
			}
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "该用户尚未注册";
	}
	

LLL:
	/*打包结果*/
	datapack->commandtype = COMMAND_SIGOUT;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		memcpy(resultinfo.resultinfo, &userinfoall, sizeof(USERINFOALL));
		resultinfo.result = OK;

		printf("log---注销成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---注销失败失败 错误信息：%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---注销申请回复发送失败\n");
	else
		printf("log---注销申请回复发送成功\n");
}

void CMSServer::CommandLoginReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);
	
	/*校验登录信息*/	
	USERINFOALL userinfoall;	
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "该账户已经登陆";
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
				/*获取并打包用户完整信息*/
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
				errorinfo = "密码错误";
				break;
			}
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "该用户尚未注册";
	}
	

LLL:
	/*打包结果*/
	datapack->commandtype = COMMAND_LOGIN;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		memcpy(resultinfo.resultinfo, &userinfoall, sizeof(USERINFOALL));
		resultinfo.result = OK;

		printf("log---登录成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---登录失败 错误信息：%s\n",resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---登录申请回复发送失败\n");
	else
		printf("log---登录申请回复发送成功\n");
}

void CMSServer::CommandLogoutReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);


	/*校验登录信息*/
	bool result = false;
	std::string errorinfo = "";
	if (_mapUserOnline.count(userinfo.userid))
	{
		MapRomoveByKey(userinfo.userid);
		result = true;
	}
	else
	{
		errorinfo = "该用户未登录";
	}

	/*打包结果*/
	datapack->commandtype = COMMAND_LOGOUT;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---退出登录成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---推出登录失败 错误信息：%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---退出登录申请回复发送失败\n");
	else
		printf("log---退出登录申请回复发送成功\n");
	
}

void CMSServer::CommandAddFriendReturn(int socket, DATAPACK *datapack)
{
	CHATINFO chatinfo;
	memcpy(&chatinfo, datapack->data, sizeof(chatinfo));
	printf("data---申请人：%d\n", chatinfo.useridfrom);
	printf("data---要添加的好友：%d\n", chatinfo.useridto);
	printf("data---打招呼：%s\n", chatinfo.info);

	/*校验好友信息*/
	USERINFO userinfo;
	bool result = false;
	std::string errorinfo = "";

	if (!_mapUserOnline.count(chatinfo.useridfrom))
	{
		errorinfo = "用户未登录";
		goto LLL;
	}

	if (chatinfo.useridfrom == chatinfo.useridto)
	{
		errorinfo = "无法添加自己";
		goto LLL;
	}

	/*查找添加的好友是否注册*/
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
			/*获取添加的好友信息*/
			std::string nickname, description;
			oJson.Get("nickname", nickname);
			oJson.Get("description", description);
			userinfo.userid = chatinfo.useridto;
			strcpy(userinfo.nickname, nickname.c_str());
			strcpy(userinfo.password, "");
			strcpy(userinfo.userdescription, description.c_str());

			result = true;
			break;
		}
	}
		
	if (!result)
	{
		errorinfo = "该用户尚未注册";
		goto LLL;
	}

	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (chatinfo.useridfrom == userid)
		{
			/*检查好友是否已经存在*/
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
				errorinfo = "好友已添加过";
			}
			break;
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "当前用户未注册";
	}
	

LLL:
	/*打包结果*/
	datapack->commandtype = COMMAND_ADDFRIEND;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;
		memcpy(resultinfo.resultinfo, &userinfo, sizeof(USERINFO));

		printf("log---添加好友成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---添加好友失败 错误信息：%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---添加好友申请回复发送失败\n");
	else
		printf("log---添加好友申请回复发送成功\n");
}

void CMSServer::CommandDelFriendReturn(int socket, DATAPACK *datapack)
{
	CHATINFO chatinfo;
	memcpy(&chatinfo, datapack->data, sizeof(chatinfo));
	printf("data---申请人：%d\n", chatinfo.useridfrom);
	printf("data---要添加的好友：%d\n", chatinfo.useridto);
	printf("data---寄语：%s\n", chatinfo.info);

	/*校验好友信息*/
	bool result = false;
	std::string errorinfo = "";

	if (!_mapUserOnline.count(chatinfo.useridfrom))
	{
		errorinfo = "用户未登录";
		goto LLL;
	}

	if (chatinfo.useridfrom == chatinfo.useridto)
	{
		errorinfo = "无法删除自己";
		goto LLL;
	}

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
				errorinfo = "你没有该好友";
			}
			break;
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = "当前用户尚未注册";
	}
	

LLL:
	/*打包结果*/
	datapack->commandtype = COMMAND_DELFRIEND;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---删除好友成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---删除好友失败 错误信息：%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---删除好友申请回复发送失败\n");
	else
		printf("log---删除好友申请回复发送成功\n");
}

void CMSServer::CommandSingleChatReturn(int socket, DATAPACK *datapack)
{
	CHATINFO chatinfo;
	memcpy(&chatinfo, datapack->data, sizeof(chatinfo));
	printf("data---发送人：%d\n", chatinfo.useridfrom);
	printf("data---接收人：%d\n", chatinfo.useridto);
	printf("data---信息：%s\n", chatinfo.info);

	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(chatinfo.useridto))
	{
		errorinfo = "好友未登录";
		goto LLL;
	}

	int sock =  _mapUserOnline[chatinfo.useridto];

	if (!SendDataPackReturn(sock, datapack))
		errorinfo = "信息发送失败";
	else
		result = true;

LLL:
	/*打包结果*/
	datapack->commandtype = COMMAND_SINGLECHAT;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;

		printf("log---发送聊天信息成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---发送聊天信息失败 错误信息：%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---单聊申请回复发送失败\n");
	else
		printf("log---单聊申请回复发送成功\n");
}

void CMSServer::CommandFriendInfoReturn(int socket, DATAPACK *datapack)
{
	USERINFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(userinfo));
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);

	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "用户未登录";
		goto LLL;
	}

	FRIENDINFO friendinfo;
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
				oJson["friend"].Get(j, friendinfo.userid[j]);
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
			if (userid == friendinfo.userid[j])
			{
				std::string nickname;
				oJson.Get("nickname", nickname);
				strcpy(friendinfo.nickname[j], nickname.c_str());
			}
		}
	}


LLL:
	/*打包结果*/
	datapack->commandtype = COMMAND_FRIENDINFO;
	datapack->datatype = RESULT_RETURN;

	RESULTINFO  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO));
	if (result)
	{
		resultinfo.result = OK;
		memcpy(resultinfo.resultinfo, &friendinfo, sizeof(FRIENDINFO));

		printf("log---获取好友信息成功\n");
	}
	else
	{
		strcpy(resultinfo.resultinfo, errorinfo.c_str());
		resultinfo.result = FAIL;

		printf("log---获取好友信息失败 错误信息：%s\n", resultinfo.resultinfo);
	}

	memcpy(datapack->data, &resultinfo, sizeof(RESULTINFO));

	/*发送结果*/
	if (!SendDataPackReturn(socket, datapack))
		printf("log---获取好友信息申请回复发送失败\n");
	else
		printf("log---获取好友信息申请回复发送成功\n");
}