#include "MSServer.h"
#include <ctime>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <stdarg.h>
#include <string.h>
#include <functional>

#ifdef __linux__
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#elif  defined(_WIN32)
#include <io.h>
#include <direct.h>
#include <windows.h>
#endif

#if _MSC_VER
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define access _access
#define mkdir _mkdir
#endif

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

std::string CMSServer::GetCurTime()
{
	time_t now = time(0);
	tm *ltm = localtime(&now);

	char timestr[64];
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", ltm);
	return timestr;
}

std::string CMSServer::GetExePath()
{
	std::string s_path;
#ifdef __linux__
	char path[1024];
	int cnt = readlink("/proc/self/exe", path, 1024);
	if (cnt < 0 || cnt >= 1024)
	{
		return NULL;
	}
	/* 最后一个'/' 后面是可执行程序名，去掉可执行程序的名字，只保留路径 不包含最后一个'/' */
	for (int i = cnt; i >= 0; --i)
	{
		if (path[i] == '/')
		{
			path[i] = '\0';
			break;
		}
	}
	s_path = std::string(path);
#elif  defined(_WIN32)
	std::string path = _pgmptr;
	s_path = path.substr(0, path.rfind("\\"));
#endif
	return s_path;
}

void CMSServer::Prtinf(unsigned short cr, const char *_format, ...)
{
	char szBuffer[65535];
	memset(szBuffer, 0x00, sizeof(szBuffer));

	va_list ap;
	va_start(ap, _format);
	try
	{
		vsnprintf(szBuffer, 65535, _format, ap);
	}
	catch (...)
	{
		//ERROR: format the string failed...
		return;
	}
	va_end(ap);

#ifdef _WIN32
	//	0 = 黑色		1 = 蓝色		2 = 绿色		3 = 淡浅蓝色 	4 = 红色		5 = 紫色		6 = 黄色		7 = 白色	
	//	8 = 灰色		9 = 淡蓝色	A = 淡绿色	B = 淡浅绿色		C = 淡红色	D = 淡紫色	E = 淡黄色	F = 亮白色
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), cr);
#endif

	std::string info = szBuffer;
	info = GetCurTime() + ":" + info;
	printf("%s", info.c_str());

#ifdef _WIN32
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0 << 4 | 0x7);
#endif
}

void CMSServer::ReadUserInfoJson()
{
	/*获取可执行文件目录*/
	std::string strexe = GetExePath();

	char userinfofile[254];
	sprintf(userinfofile, "%s/User/userinfo.json", strexe.c_str());

	if (access(userinfofile, 6) != 0)
	{
		/*创建User目录*/
		std::string userpath = strexe + "/User";
		if (access(userpath.c_str(), 0) != 0)
		{
#ifdef __linux__
			mkdir(userpath.c_str(), S_IRWXU);
#elif  defined(_WIN32)
			mkdir(userpath.c_str());
#endif
		}
	}

	std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(userinfofile, std::ios::in);
	std::string strjson;
	std::string strtemp;
	while (*content >> strtemp) {
		strjson += strtemp;
	}
	_oJsonuserinfo.Parse(strjson);

	Prtinf(0x9 ,"%s\n", _oJsonuserinfo.ToFormattedString().c_str());
}

void CMSServer::SaveUserInfoJson()
{
	/*获取可执行文件目录*/
	std::string strexe = GetExePath();

	char userinfofile[254];
	sprintf(userinfofile, "%s/User/userinfo.json", strexe.c_str());

	if (access(userinfofile, 6) != 0)
	{
		/*创建User目录*/
		std::string userpath = strexe + "/User";
		if (access(userpath.c_str(), 0) != 0)
		{
#ifdef __linux__
			mkdir(userpath.c_str(), S_IRWXU);
#elif  defined(_WIN32)
			mkdir(userpath.c_str());
#endif
		}
	}

	std::string strjson = _oJsonuserinfo.ToFormattedString();
	std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(userinfofile, std::ios::out);
	*content << strjson;

	Prtinf(0x9 ,"%s\n", strjson.c_str());
}

bool CMSServer::MapRomoveByValue(int value)
{
	bool res = false;
	for (auto iter = _mapUserOnline.begin(); iter != _mapUserOnline.end(); iter++)
	{
		if (iter->second == value)
		{
			LoginStateNotify(COMMAND_LOGOUT, iter->first);
			_mapUserOnline.erase(iter->first);
			res = true;
			break;
		}
	}
	return res;
}

bool CMSServer::MapRomoveByKey(unsigned int key)
{
	LoginStateNotify(COMMAND_LOGOUT, key);
	_mapUserOnline.erase(key);

	return true;
}

bool CMSServer::MapInsertData(unsigned int key, int value)
{
	_mapUserOnline.insert(std::pair<unsigned int, int>(key, value));
	LoginStateNotify(COMMAND_LOGIN, key);

	return true;
}

void CMSServer::Update(int socket)
{
	socketevent event = _socket->getsocketevent();
	switch (event)
	{
	case clientrecv: Prtinf(0x6, "user---socket:%d 客户端收到信息\n", socket); break;
	case clientaccpet:Prtinf(0x6, "user---socket:%d 客户端连接\n", socket); break;
	case clientdiscon:Prtinf(0x6, "user---socket:%d 客户端断开连接\n", socket); MapRomoveByValue(socket); break;
	case servercolse:Prtinf(0x6, "user---socket:%d 服务器关闭\n", socket); _oJsonuserinfo.Clear(); _mapUserOnline.clear(); break;
	case datanodefine:Prtinf(0x6, "user---socket:%d 未定义数据\n", socket); UndefineDataRequst(socket); break;
	case serverrecv:Prtinf(0x6, "user---socket:%d 服务器收到消息\n", socket); RecvDataProcess(socket); break;
	default:
		break;
	}
}

void CMSServer::UndefineDataRequst(int socket)
{
	char *buffer = new char[DATAPACKETSIZE];
	_socket->get_recvbuf(&buffer);

	Prtinf(0x4, "log---undefine info:%s", buffer);

	if (strstr(buffer, "HTTP"))
	{
		/*解析请求*/
		char filename[10] = { 0 };
		sscanf(buffer, "GET /%s", filename);
		printf("解析到的文件名是%s\n", filename);

		/*设置内容类型*/
		char *mime = nullptr;
		if (strstr(filename, "html"))
		{
			mime = (char*)"text/html";
		}
		else if (strstr(filename, ".png"))
		{
			mime = (char*)"img/png";
		}
		else if (strstr(filename, ".jpg"))
		{
			mime = (char*)"img/jpg";
		}

		/*应答包头*/
		char head[128];
		sprintf(head, "HTTP/1.1 200 OK\r\nConrent-Type:%s\r\n\r\n", mime);
		int headlen = strlen(head);

		/*应答包内容*/
		char file[128];
		sprintf(file, "%s/%s", GetExePath().c_str(), filename);
		std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(file, std::ios::in | std::ios::binary);

		const int datalen = 1024;
		char buffer[datalen];
		memset(buffer, 0, datalen);
		memcpy(buffer, head, headlen);
		while (!content->eof())
		{
			content->read(buffer + headlen, datalen - headlen);
			/*发送应答包*/
			if (0 == _socket->send_skt(socket, (char*)buffer, datalen))
			{
				headlen = 0;
			}
			
		}
	}
	delete[] buffer;
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
				Prtinf(0x5, "%s---收到注册申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSiginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_SIGOUT: /*注销*/
			{
				Prtinf(0x5, "%s---收到注销申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSigoutReturn, this, std::placeholders::_1, std::placeholders::_2);	
			}
			break;
		case COMMAND_LOGIN:/*登入*/
			{
				Prtinf(0x5, "%s---收到登录申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandLoginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_LOGOUT:/*登出*/
			{
				Prtinf(0x5, "%s---收到退出登录申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandLogoutReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDFRIEND:/*添加好友*/
			{
				Prtinf(0x5, "%s---收到添加好友申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandAddFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_DELFRIEND:/*删除好友*/
			{
				Prtinf(0x5, "%s---收到删除好友申请！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandDelFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDGROUND:/*加入群聊*/
			{
				Prtinf(0x5, "%s---收到加入群聊申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				Prtinf(0xd, "data---申请人:%d\n", chatinfo.useridfrom);
				Prtinf(0xd, "data---要加入的群:%d\n", chatinfo.useridto);
				Prtinf(0xd, "data---打招呼:%s\n", chatinfo.info);
			}
			break;
		case COMMAND_DELGROUND:/*退出群聊*/
			{
				Prtinf(0x5, "%s---收到退出群聊申请！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				Prtinf(0xd, "data---申请人:%d\n", chatinfo.useridfrom);
				Prtinf(0xd, "data---要退出的群:%d\n", chatinfo.useridto);
				Prtinf(0xd, "data---寄语:%s\n", chatinfo.info);
			}
			break;
		case COMMAND_SINGLECHAT:/*单聊*/
			{
				Prtinf(0x5, "%s---收到单聊信息！\n", GetCurTime().c_str());
				CommandReturn = std::bind(&CMSServer::CommandSingleChatReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_GROUPCHAT:/*群聊*/
			{
				Prtinf(0x5, "%s---收到群聊信息！\n", GetCurTime().c_str());
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				Prtinf(0xd, "data---发送人:%d\n", chatinfo.useridfrom);
				Prtinf(0xd, "data---接收群:%d\n", chatinfo.useridto);
				Prtinf(0xd, "data---信息:%s\n", chatinfo.info);
			}
			break;
		case COMMAND_FRIENDINFO:/*群聊*/
			{
				Prtinf(0x5, "%s---收到获取好友信息申请！\n", GetCurTime().c_str());
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
		Prtinf(0x8, "log---命令%d回复失败\n", type);
	}
	else
	{
		Prtinf(0x8, "log---命令%d回复成功\n",type);
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
	Prtinf(0xd, "data---昵称:%s\n", sigininfo.nickname);
	Prtinf(0xd, "data---账户:%d\n", sigininfo.userid);
	Prtinf(0xd, "data---密码:%s\n", sigininfo.password);
	Prtinf(0xd, "data---签名:%s\n", sigininfo.userdescription);

	/*校验注册信息*/
	int usersize = 0;
	bool result = true;
	std::string errorinfo = "";
	if (sigininfo.userid < 99999 || sigininfo.userid >999999999)
	{
		result = false;
		errorinfo = "账号不符合规定";
		goto LLL;
	}

	usersize = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < usersize; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		std::string password;
		oJson.Get("userid", userid);
		oJson.Get("password", password);

		if (sigininfo.userid == userid)
		{
			result = false;
			break;
		}
	}
	if (!result)
	{
		errorinfo = "该账号已经注册，请直接登录";
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
		Prtinf(0x8, "log---注册成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---注册失败失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandSigoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	Prtinf(0xd, "data---账户:%d\n", userinfo.userid);
	Prtinf(0xd, "data---密码:%s\n", userinfo.password);
	
	/*校验登录信息*/
	int usersize = 0;
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "该账户尚未登陆";
		goto LLL;
	}
	
	usersize = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < usersize; ++i)
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
		Prtinf(0x8, "log---注销成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---注销失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLoginReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	Prtinf(0xd, "data---账户:%d\n", userinfo.userid);
	Prtinf(0xd, "data---密码:%s\n", userinfo.password);
	
	/*校验登录信息*/	
	int usersize = 0;
	USERINFO_RETURN userinfo_return;
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "该账户已经登陆";
		goto LLL;
	}

	usersize = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < usersize; ++i)
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
				/*获取并打包用户信息*/
				std::string nickname, description;
				oJson.Get("nickname", nickname);
				oJson.Get("description", description);
				strcpy(userinfo_return.nickname, nickname.c_str());
				strcpy(userinfo_return.userdescription, description.c_str());

				MapInsertData(userid, socket);
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
		Prtinf(0x8, "log---登录成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---登录失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLogoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	Prtinf(0xd, "data---账户:%d\n", userinfo.userid);
	Prtinf(0xd, "data---密码:%s\n", userinfo.password);

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
		goto LLL;
	}

LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		Prtinf(0x8, "log---退出登录成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---退出登录失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
	
}

void CMSServer::CommandAddFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friend_info;
	memcpy(&friend_info, datapack->data, sizeof(FRIEND_INFO));
	Prtinf(0xd, "data---申请人:%d\n", friend_info.userid);
	Prtinf(0xd, "data---要添加的好友:%d\n", friend_info.friendid);
	Prtinf(0xd, "data---打招呼:%s\n", friend_info.info);

	/*校验好友信息*/
	int usersize = 0;
	FRIENDINFO_RETURN friendinfo;
	bool result = false;
	std::string errorinfo = "";

	if (friend_info.userid == friend_info.friendid)
	{
		errorinfo = "无法添加自己";
		goto LLL;
	}

	/*查找添加的好友是否注册*/
	usersize= _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < usersize; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (friend_info.friendid == userid)
		{
			/*获取添加的好友信息*/
			friendinfo.friendid = friend_info.friendid;
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

	for (int i = 0; i < usersize; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (friend_info.userid == userid)
		{
			/*检查好友是否已经存在*/
			int sz = oJson["friend"].GetArraySize();
			for (int j = 0; j < sz; j++)
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
				//todo 添加好友这个功能需要改为对方同意的机制
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

	//int sock = _mapUserOnline[friend_info.friendid];

	//if (!SendDataPack(sock, datapack))
	//	errorinfo = "信息发送失败";
	//else
	//	result = true;

LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &friendinfo;
		datasize = sizeof(FRIENDINFO_RETURN);
		Prtinf(0x8, "log---登录成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---登录失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandDelFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friendinfo;
	memcpy(&friendinfo, datapack->data, sizeof(FRIEND_INFO));
	Prtinf(0xd, "data---申请人:%d\n", friendinfo.userid);
	Prtinf(0xd, "data---要删除的好友:%d\n", friendinfo.friendid);
	Prtinf(0xd, "data---寄语:%s\n", friendinfo.info);

	/*校验好友信息*/
	int usersize = 0;
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

	usersize = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < usersize; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (friendinfo.userid == userid)
		{
			int sz = oJson["friend"].GetArraySize();
			for (int j = 0; j < sz; j++)
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
		Prtinf(0x8, "log---删除好友成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---删除好友失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandSingleChatReturn(int socket, DATA_PACK *datapack)
{
	unsigned int userid, friendid;
	std::string info,bucket,object;
	if (datapack->datatype == CHAT_TEXT)
	{
		CHATINFO chatinfo;
		memcpy(&chatinfo, datapack->data, sizeof(chatinfo));
		Prtinf(0xd, "data---发送人:%d\n", chatinfo.useridfrom);
		Prtinf(0xd, "data---接收人:%d\n", chatinfo.useridto);
		Prtinf(0xd, "data---信息:%s\n", chatinfo.info);

		userid = chatinfo.useridfrom;
		friendid = chatinfo.useridto;
		info = chatinfo.info;
	}
	else if (datapack->datatype == CHAT_FILE)
	{
		OSSFILEINFO fileinfo;
		memcpy(&fileinfo, datapack->data, sizeof(fileinfo));
		Prtinf(0xd, "data---发送人:%d\n", fileinfo.useridfrom);
		Prtinf(0xd, "data---接收人:%d\n", fileinfo.useridto);
		Prtinf(0xd, "data---bucket:%s\n", fileinfo.bucket);
		Prtinf(0xd, "data---object:%s\n", fileinfo.object);

		userid = fileinfo.useridfrom;
		friendid = fileinfo.useridto;
		bucket = fileinfo.bucket;
		object = fileinfo.object;
	}
	

	bool result = false;
	std::string errorinfo = "";

	bool friendexit = false;
	int usersize = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < usersize; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		if (friendid == userid)
		{
			int sz = oJson["friend"].GetArraySize();
			for (int j = 0; j < sz; j++)
			{
				unsigned int friendid;
				oJson["friend"].Get(j, friendid);
				if (userid == friendid)
				{
					result = true;
					break;
				}
			}
			if (!result)
			{
				errorinfo = "你还不是对方的好友";
			}
			friendexit = true;
			break;
		}
	}
	if (!friendexit)
	{
		errorinfo = "对方已经注销账号";
		goto LLL;
	}

	if (!_mapUserOnline.count(friendid))
	{
		errorinfo = "好友未登录,信息将缓存，在好友上线后发送";

		int datasize = 0;
		if (datapack->datatype == CHAT_TEXT)
			datasize = sizeof(CHATINFO);
		else if (datapack->datatype == CHAT_FILE)
			datasize = sizeof(OSSFILEINFO);
		SaveCacheInfo((void*)datapack->data, datasize);

		goto LLL;
	}
	else
	{
		if (!SendDataPack(_mapUserOnline[friendid], datapack))
			errorinfo = "信息发送失败";
		else
			result = true;
	}

LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		Prtinf(0x8, "log---发送聊天信息成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---发送聊天信息失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandFriendInfoReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	Prtinf(0xd, "data---账户:%d\n", userinfo.userid);
	Prtinf(0xd, "data---密码:%s\n", userinfo.password);

	int usersize = 0;
	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = "用户未登录";
		goto LLL;
	}

	ALLRIENDINFO_RETURN friendinfo;
	friendinfo.isonline &= 0x0;
	/*获取好友列表*/
	usersize = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < usersize; ++i)
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

	/*获取好友信息*/
	for (int i = 0; i < usersize; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int userid;
		oJson.Get("userid", userid);
		for (size_t j = 0; j < friendinfo.friendsize; j++)
		{
			if (userid == friendinfo.friendid[j])
			{
				friendinfo.isonline |= (_mapUserOnline.count(userid) << j);
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
		Prtinf(0x8, "log---获取好友成功\n");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		Prtinf(0x8, "log---获取好友失败 错误信息:%s\n", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);

	if (result)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		SenCacheInfo(userinfo.userid);
	}
}

void CMSServer::SaveCacheInfo(void* data, int size)
{
	unsigned int userid, friendid;
	std::string info, bucket, object;
	int type = -1;

	if (size == sizeof(CHATINFO))
	{ 
		type = CHAT_TEXT;
		CHATINFO chatinfo;
		memset(&chatinfo, 0, sizeof(CHATINFO));
		memcpy(&chatinfo, (char*)data, sizeof(chatinfo));
		userid = chatinfo.useridfrom;
		friendid = chatinfo.useridto;
		info = chatinfo.info;
	}
	else if (size == sizeof(OSSFILEINFO))
	{
		type = CHAT_FILE;
		OSSFILEINFO fileinfo;
		memset(&fileinfo, 0, sizeof(OSSFILEINFO));
		memcpy(&fileinfo, (char*)data, sizeof(fileinfo));
		userid = fileinfo.useridfrom;
		friendid = fileinfo.useridto;
		bucket = fileinfo.bucket;
		object = fileinfo.object;
	}
	else if (size == sizeof(FRIEND_INFO))
	{
		FRIEND_INFO friendinfo;
		memset(&friendinfo, 0, sizeof(FRIEND_INFO));
		memcpy(&friendinfo, (char*)data, sizeof(friendinfo));
		userid = friendinfo.userid;
		friendid = friendinfo.friendid;
		info = friendinfo.info;
	}


	/*缓存文件json*/
	neb::CJsonObject cachejson;

	/*获取可执行文件目录*/
	std::string strexe = GetExePath();

	char cachefile[254];
	sprintf(cachefile, "%s/User/%d/cachefile.json", strexe.c_str(), friendid);

	if (access(cachefile, 6) != 0)
	{
		/*创建User目录*/
		std::string userpath = strexe + "/User";
		if (access(userpath.c_str(), 0) != 0)
		{
#ifdef __linux__
			mkdir(userpath.c_str(), S_IRWXU);
#elif  defined(_WIN32)
			mkdir(userpath.c_str());
#endif
		}
		/*创建用户ID目录*/
		char useridpath[254];
		sprintf(useridpath, "%s/%d", userpath.c_str(), friendid);
		if (access(useridpath, 0) != 0)
		{
#ifdef __linux__
			mkdir(useridpath, S_IRWXU);
#elif  defined(_WIN32)
			mkdir(useridpath);
#endif
		}

		/*创建cachejson*/
		char struserid[16];
		snprintf(struserid, sizeof(struserid), "%d", userid);

		neb::CJsonObject friendinfo;
		friendinfo.AddEmptySubArray("info");
		friendinfo.AddEmptySubArray("file");
		friendinfo.AddEmptySubArray("new");
		if (type == CHAT_TEXT)
		{
			friendinfo["info"].Add(info);
		}
		else if (type == CHAT_FILE)
		{
			neb::CJsonObject fileinfo;
			fileinfo.Add("bucket", bucket);
			fileinfo.Add("object", object);
			friendinfo["file"].Add(fileinfo);
		}
		else
		{
			friendinfo["new"].Add(info);
		}
		cachejson.Add(struserid, friendinfo);

		/*写入文件*/
		std::string strjson = cachejson.ToFormattedString();
		std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(cachefile, std::ios::out);
		*content << strjson;

	}
	else
	{
		/*读取cachejson*/
		std::shared_ptr<std::iostream> contentread = std::make_shared<std::fstream>(cachefile, std::ios::in);
		std::string strjson;
		std::string strtemp;
		while (*contentread >> strtemp) {
			strjson += strtemp;
		}
		cachejson.Parse(strjson);

		/*修改cachejson*/
		neb::CJsonObject friendinfo;
		char struserid[16];
		snprintf(struserid, sizeof(struserid), "%d", userid);

		cachejson.Get(struserid, friendinfo);

		if (type == CHAT_TEXT)
		{
			friendinfo["info"].Add(info);
		}
		else if (type == CHAT_FILE)
		{
			neb::CJsonObject fileinfo;
			fileinfo.Add("bucket", bucket);
			fileinfo.Add("object", object);
			friendinfo["file"].Add(fileinfo);
		}
		else
		{
			friendinfo["new"].Add(info);
		}
		cachejson.Replace(struserid, friendinfo);

		/*写入文件*/
		strjson = cachejson.ToFormattedString();
		std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(cachefile, std::ios::out);
		*content << strjson;
	}
}

void CMSServer::SenCacheInfo(unsigned int userid)
{
	/*获取可执行文件目录*/
	std::string strexe = GetExePath();
	char cachefile[254];
	sprintf(cachefile, "%s/User/%d/cachefile.json", strexe.c_str(), userid);

	if (access(cachefile, 6) == 0)
	{
		/*缓存文件json*/
		neb::CJsonObject cachejson;

		/*读取缓存文件*/
		std::shared_ptr<std::iostream> contentread = std::make_shared<std::fstream>(cachefile, std::ios::in);
		std::string strjson;
		std::string strtemp;
		while (*contentread >> strtemp) {
			strjson += strtemp;
		}
		cachejson.Parse(strjson);

		std::string strfriendkey;
		while (cachejson.GetKey(strfriendkey))
		{
			neb::CJsonObject friendinfo;
			cachejson.Get(strfriendkey, friendinfo);

			int infosz = friendinfo["info"].GetArraySize();
			for (int i = 0; i < infosz; ++i)
			{
				CHATINFO chatinfo;
				unsigned int friendid = atoi(strfriendkey.c_str());
				std::string info;
				friendinfo["info"].Get(i, info);

				chatinfo.useridfrom = friendid;
				chatinfo.useridto = userid;
				strcpy(chatinfo.info, info.c_str());

				DATA_PACK datapack;
				datapack.commandtype = COMMAND_SINGLECHAT;
				datapack.datatype = CHAT_TEXT;

				memcpy(datapack.data, &chatinfo, sizeof(CHATINFO));

				SendDataPack(_mapUserOnline[userid], &datapack);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			int filesz = friendinfo["file"].GetArraySize();
			for (int i = 0; i < filesz; ++i)
			{
				OSSFILEINFO chatinfo;
				unsigned int friendid = atoi(strfriendkey.c_str());
				neb::CJsonObject fileinfo;
				friendinfo["file"].Get(i, fileinfo);
				std::string bucket, object;
				fileinfo.Get("bucket", bucket);
				fileinfo.Get("object", object);

				chatinfo.useridfrom = friendid;
				chatinfo.useridto = userid;
				strcpy(chatinfo.bucket, bucket.c_str());
				strcpy(chatinfo.object, object.c_str());

				DATA_PACK datapack;
				datapack.commandtype = COMMAND_SINGLECHAT;
				datapack.datatype = CHAT_FILE;

				memcpy(datapack.data, &chatinfo, sizeof(OSSFILEINFO));

				SendDataPack(_mapUserOnline[userid], &datapack);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			int friendsz = friendinfo["new"].GetArraySize();
			for (int i = 0; i < filesz; ++i)
			{
				FRIEND_INFO friend_info;
				unsigned int friendid = atoi(strfriendkey.c_str());
				std::string info;
				friendinfo["new"].Get(i, info);

				friend_info.friendid = userid;
				friend_info.userid = friendid;
				strcpy(friend_info.info, info.c_str());

				DATA_PACK datapack;
				datapack.commandtype = COMMAND_ADDFRIEND;
				datapack.datatype = CHAT_TEXT;

				memcpy(datapack.data, &friend_info, sizeof(FRIEND_INFO));

				SendDataPack(_mapUserOnline[userid], &datapack);
			}
		}
	}
	remove(cachefile);
}

void CMSServer::LoginStateNotify(int state,unsigned int userid)
{
	Prtinf(0x8, "log---通知好友%s线\n", state == COMMAND_LOGIN ? "上" : "下");

	DATA_PACK datapack;
	memset(&datapack, 0, sizeof(DATA_PACK));
	/*打包结果*/
	datapack.commandtype = COMMANDTYPE(state);
	datapack.datatype = NOTIFY_INFO;

	/*通知好友自己下线信息*/
	int size = _oJsonuserinfo["userinfo"].GetArraySize();
	for (int i = 0; i < size; ++i)
	{
		neb::CJsonObject oJson;
		_oJsonuserinfo["userinfo"].Get(i, oJson);
		unsigned int nuserid;
		std::string password;
		oJson.Get("userid", nuserid);
		oJson.Get("password", password);

		if (userid == nuserid)
		{
			/*打包自己的信息*/
			FRIENDINFO_RETURN friendinfo;
			std::string nickname, description;
			oJson.Get("nickname", nickname);
			oJson.Get("description", description);
			friendinfo.friendid = userid;
			strcpy(friendinfo.nickname, nickname.c_str());
			strcpy(friendinfo.userdescription, description.c_str());
			memcpy(datapack.data, &friendinfo, sizeof(RESULTINFO_RETURN));

			/*发送给好友*/
			int sz = oJson["friend"].GetArraySize();
			for (int j = 0; j < sz; j++)
			{
				unsigned int friendid;
				oJson["friend"].Get(j, friendid);

				if (_mapUserOnline.count(friendid))
				{
					SendDataPack(_mapUserOnline[friendid], &datapack);
				}
			}
			break;
		}
	}
}