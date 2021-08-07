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
		std::function<void(int,DATA_PACK*)> CommandReturn;

		DATA_PACK datapack;
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

void CMSServer::SendDataPackReturn(int socket, COMMANDTYPE type, RESULT result, void* data, int size)
{
	DATA_PACK datapack;
	memset(&datapack, 0, sizeof(DATA_PACK));

	RESULTINFO_RETURN  resultinfo;
	memset(&resultinfo, 0, sizeof(RESULTINFO_RETURN));
	resultinfo.result = result;
	memcpy(resultinfo.resultinfo, (char*)data, size);

	/*打包结果*/
	datapack.commandtype = type;
	datapack.datatype = RESULT_RETURN;
	memcpy(datapack.data, &resultinfo, sizeof(RESULTINFO_RETURN));

	/*发送结果*/
	char *buff = new char[sizeof(DATA_PACK)];
	memset(buff, 0, sizeof(DATA_PACK));
	memcpy(buff, &datapack, sizeof(DATA_PACK));
	if (0 != _socket->send_skt(socket, (char*)buff, sizeof(DATA_PACK)))
	{
		printf("log---命令%d回复失败\n", type);
	}
	else
	{
		printf("log---命令%d回复成功\n",type);
	}
	delete[] buff;
}

bool CMSServer::SendDataPack(int socket, DATA_PACK *datapack)
{
	bool result = false;
	/*发送结果*/
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
	printf("data---昵称：%s\n", sigininfo.nickname);
	printf("data---账户：%d\n", sigininfo.userid);
	printf("data---密码：%s\n", sigininfo.password);
	printf("data---签名：%s\n", sigininfo.userdescription);

	/*校验注册信息*/
	bool result = true;
	std::string errorinfo = "";
	if (sigininfo.userid < 99999 || sigininfo.userid >999999999)
	{
		result = false;
		errorinfo = "账号不符合规定";
		goto LLL;
	}
	
	/*写入注册信息*/
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
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;;
	if (result)
	{
		data = nullptr;
		printf("log---注册成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---注册失败失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandSigoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);
	
	/*校验登录信息*/
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
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		printf("log---注销成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---注销失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLoginReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);
	
	/*校验登录信息*/	
	USERINFO_RETURN userinfo_return;
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
				/*获取并打包用户信息*/
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
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &userinfo_return;
		datasize = sizeof(USERINFO_RETURN);
		printf("log---登录成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---登录失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLogoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
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

LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		printf("log---退出登录成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---退出登录失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
	
}

void CMSServer::CommandAddFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friend_info;
	memcpy(&friend_info, datapack->data, sizeof(FRIEND_INFO));
	printf("data---申请人：%d\n", friend_info.userid);
	printf("data---要添加的好友：%d\n", friend_info.friendid);
	printf("data---打招呼：%s\n", friend_info.info);

	/*校验好友信息*/
	FRIENDINFO_RETURN friendinfo;
	bool result = false;
	std::string errorinfo = "";

	if (friend_info.userid == friend_info.friendid)
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
		if (friend_info.friendid == userid)
		{
			/*获取添加的好友信息*/
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
		errorinfo = "该用户尚未注册";
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
			/*检查好友是否已经存在*/
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
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &friendinfo;
		datasize = sizeof(FRIENDINFO_RETURN);
		printf("log---登录成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---登录失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandDelFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friendinfo;
	memcpy(&friendinfo, datapack->data, sizeof(FRIEND_INFO));
	printf("data---申请人：%d\n", friendinfo.userid);
	printf("data---要添加的好友：%d\n", friendinfo.friendid);
	printf("data---寄语：%s\n", friendinfo.info);

	/*校验好友信息*/
	bool result = false;
	std::string errorinfo = "";

	if (!_mapUserOnline.count(friendinfo.userid))
	{
		errorinfo = "用户未登录";
		goto LLL;
	}

	if (friendinfo.userid == friendinfo.friendid)
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
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		printf("log---删除好友成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---删除好友失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandSingleChatReturn(int socket, DATA_PACK *datapack)
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

	if (!SendDataPack(sock, datapack))
		errorinfo = "信息发送失败";
	else
		result = true;

LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		printf("log---发送聊天信息成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---发送聊天信息失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandFriendInfoReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	printf("data---账户：%d\n", userinfo.userid);
	printf("data---密码：%s\n", userinfo.password);

	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "用户未登录";
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
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &friendinfo;
		datasize = sizeof(ALLRIENDINFO_RETURN);
		printf("log---获取好友成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		printf("log---获取好友失败 错误信息：%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}