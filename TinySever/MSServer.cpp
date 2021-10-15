#include <ctime>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <stdarg.h>
#include <string.h>
#include <functional>
#include "MSServer.h"
#include "log.h"

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
	std::string logpath = GetExePath() +"/log";
	if (access(logpath.c_str(), 0) != 0)
	{
#ifdef __linux__
		mkdir(logpath.c_str(), S_IRWXU);
#elif  defined(_WIN32)
		mkdir(logpath.c_str());
#endif
	}
	std::string exname = "Server";
	
	/*初始化log*/
	LOG_INIT(logpath.c_str(), exname.c_str(), 100);

	this->_socket = sub;
	ReadUserInfoJson();

}

CMSServer::~CMSServer()
{
	_oJsonuserinfo.Clear();
	_mapUserOnline.clear();
	/*释放log*/
	LOG_UNINIT;
}

std::string CMSServer::GBKToUTF8(const std::string& strGBK)
{
#if _MSC_VER
	std::string strOutUTF8 = "";
	WCHAR * str1;
	int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
	str1 = new WCHAR[n];
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	char * str2 = new char[n];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
	strOutUTF8 = str2;
	delete[]str1;
	str1 = NULL;
	delete[]str2;
	str2 = NULL;
	return strOutUTF8;
#else
	return strGBK;
#endif
}

std::string CMSServer::UTF8ToGBK(std::string &strUtf8)
{
#if _MSC_VER
	std::string strOutGBK = "";
	int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, NULL, 0);
	WCHAR *wszGBK = new WCHAR[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, wszGBK, len);
	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char *pszGBK = new char[len + 1];
	memset(pszGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, pszGBK, len, NULL, NULL);
	strOutGBK = pszGBK;
	delete[]pszGBK;
	delete[]wszGBK;
	return strOutGBK;
#else
	return strUtf8;
#endif
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
	/*最后一个'/' 后面是可执行程序名，去掉可执行程序的名字，只保留路径*/
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

void CMSServer::PrintfInfo(unsigned short cr, const char *_format, ...)
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
	LOG_INFO("%s",info.c_str());
	info = GetCurTime() + ":" + info + "\n";
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

	PrintfInfo(0x9 ,"%s", _oJsonuserinfo.ToFormattedString().c_str());
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

	PrintfInfo(0x9 ,"%s", strjson.c_str());
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
	case clientrecv: PrintfInfo(0x6, "user---socket:%d Client receives information", socket); break;
	case clientaccpet:PrintfInfo(0x6, "user---socket:%d Client connection", socket); break;
	case clientdiscon:PrintfInfo(0x6, "user---socket:%d Client disconnected", socket); MapRomoveByValue(socket); break;
	case servercolse:PrintfInfo(0x6, "user---socket:%d Server shut down", socket); _oJsonuserinfo.Clear(); _mapUserOnline.clear(); break;
	case datanodefine:PrintfInfo(0x6, "user---socket:%d Undefined data", socket); UndefineDataRequst(); break;
	case serverrecv:PrintfInfo(0x6, "user---socket:%d Server received message", socket); RecvDataProcess(); break;
	default:
		break;
	}
}

void CMSServer::UndefineDataRequst()
{
	int clientsocket;
	char *buffer = new char[DATAPACKETSIZE];
	_socket->get_recvbuf(clientsocket, &buffer);

	PrintfInfo(0x4, "log---undefine info:%s", buffer);

	if (strstr(buffer, "HTTP"))
	{
		/*解析请求*/
		char filename[10] = { 0 };
		sscanf(buffer, "GET /%s", filename);
		PrintfInfo(0x6, "filename: %s", filename);

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
		sprintf(head, "HTTP/1.1 200 OK\r\nContent-Type:%s;;charset=utf-8\r\n\r\n", mime);
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
			if (0 == _socket->send_skt(clientsocket, (char*)buffer, datalen))
			{
				headlen = 0;
			}	
		}
	}
	delete[] buffer;
}

void CMSServer::RecvDataProcess()
{
	int clientsocket;
	char *buffer = new char[DATAPACKETSIZE];
	if (_socket->get_recvbuf(clientsocket, &buffer))
	{
		std::function<void(int,DATA_PACK*)> CommandReturn;

		DATA_PACK datapack;
		memcpy(&datapack, buffer, sizeof(datapack));

		switch (COMMANDTYPE(datapack.commandtype))
		{
		case COMMAND_SIGIN: /*注册*/
			{
				PrintfInfo(0x5, "Receipt of sigin application!");
				CommandReturn = std::bind(&CMSServer::CommandSiginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_SIGOUT: /*注销*/
			{
				PrintfInfo(0x5, "Receipt of sigout application!");
				CommandReturn = std::bind(&CMSServer::CommandSigoutReturn, this, std::placeholders::_1, std::placeholders::_2);	
			}
			break;
		case COMMAND_LOGIN:/*登入*/
			{
				PrintfInfo(0x5, "Receipt of login application!!");
				CommandReturn = std::bind(&CMSServer::CommandLoginReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_LOGOUT:/*登出*/
			{
				PrintfInfo(0x5, "Receipt of logout application!!");
				CommandReturn = std::bind(&CMSServer::CommandLogoutReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDFRIEND:/*添加好友*/
			{
				PrintfInfo(0x5, "Receipt of add friend application!!");
				CommandReturn = std::bind(&CMSServer::CommandAddFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_DELFRIEND:/*删除好友*/
			{
				PrintfInfo(0x5, "Receipt of del friend application!!");
				CommandReturn = std::bind(&CMSServer::CommandDelFriendReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_ADDGROUND:/*加入群聊*/
			{
				PrintfInfo(0x5, "收到加入群聊申请!");
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				PrintfInfo(0xd, "data---申请人:%d", chatinfo.useridfrom);
				PrintfInfo(0xd, "data---要加入的群:%d", chatinfo.useridto);
				PrintfInfo(0xd, "data---打招呼:%s", chatinfo.info);
			}
			break;
		case COMMAND_DELGROUND:/*退出群聊*/
			{
				PrintfInfo(0x5, "收到退出群聊申请！");
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				PrintfInfo(0xd, "data---申请人:%d", chatinfo.useridfrom);
				PrintfInfo(0xd, "data---要退出的群:%d", chatinfo.useridto);
				PrintfInfo(0xd, "data---寄语:%s", chatinfo.info);
			}
			break;
		case COMMAND_SINGLECHAT:/*单聊*/
			{
				PrintfInfo(0x5, "Receipt of chat application!!");
				CommandReturn = std::bind(&CMSServer::CommandSingleChatReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		case COMMAND_GROUPCHAT:/*群聊*/
			{
				PrintfInfo(0x5, "收到群聊信息!");
				CHATINFO chatinfo;
				memcpy(&chatinfo, datapack.data, sizeof(chatinfo));
				PrintfInfo(0xd, "data---发送人:%d", chatinfo.useridfrom);
				PrintfInfo(0xd, "data---接收群:%d", chatinfo.useridto);
				PrintfInfo(0xd, "data---信息:%s", chatinfo.info);
			}
			break;
		case COMMAND_FRIENDINFO:/*好友信息*/
			{
				PrintfInfo(0x5, "Receipt of friend info application!!");
				CommandReturn = std::bind(&CMSServer::CommandFriendInfoReturn, this, std::placeholders::_1, std::placeholders::_2);
			}
			break;
		default:
			break;
		}
		CommandReturn(clientsocket, &datapack);
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
		PrintfInfo(0x8, "log---Command:%d Reply failed", type);
	}
	else
	{
		PrintfInfo(0x8, "log---Command:%d Reply successfully",type);
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
	PrintfInfo(0xd, "data---nickname:%s", sigininfo.nickname);
	PrintfInfo(0xd, "data---Account:%d", sigininfo.userid);
	PrintfInfo(0xd, "data---Password:%s", sigininfo.password);
	PrintfInfo(0xd, "data---Signature:%s", sigininfo.userdescription);

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
		PrintfInfo(0x8, "log---Sigin successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Sigin failed error info:%s", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandSigoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	PrintfInfo(0xd, "data---Account:%d", userinfo.userid);
	PrintfInfo(0xd, "data---Password:%s", userinfo.password);
	
	/*校验登录信息*/
	int usersize = 0;
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = GBKToUTF8("该账户尚未登陆");
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
				errorinfo = GBKToUTF8("密码错误");
			}
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = GBKToUTF8("该用户尚未注册");
	}
	
LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		PrintfInfo(0x8, "log---Sigout successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Sigout failed error info:%s", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLoginReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	PrintfInfo(0xd, "data---Account:%d", userinfo.userid);
	PrintfInfo(0xd, "data---Password:%s", userinfo.password);
	
	/*校验登录信息*/	
	int usersize = 0;
	USERINFO_RETURN userinfo_return;
	std::string stpassword = userinfo.password;
	bool result = false;
	std::string errorinfo = "";
	if (_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = GBKToUTF8("该账户已经登陆");
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
				errorinfo = GBKToUTF8("密码错误");
				break;
			}
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = GBKToUTF8("该用户尚未注册");
	}
LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = &userinfo_return;
		datasize = sizeof(USERINFO_RETURN);
		PrintfInfo(0x8, "log---Login successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Login failed error info:%s", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandLogoutReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	PrintfInfo(0xd, "data---Account:%d", userinfo.userid);
	PrintfInfo(0xd, "data---Password:%s", userinfo.password);

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
		errorinfo = GBKToUTF8("该用户未登录");
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
		PrintfInfo(0x8, "log---Logout successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Logout failed error info:%s", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
	
}

void CMSServer::CommandAddFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friend_info;
	memcpy(&friend_info, datapack->data, sizeof(FRIEND_INFO));
	PrintfInfo(0xd, "data---Account:%d", friend_info.userid);
	PrintfInfo(0xd, "data---Friend:%d", friend_info.friendid);
	PrintfInfo(0xd, "data---Info:%s", friend_info.info);

	/*校验好友信息*/
	int usersize = 0;
	FRIENDINFO_RETURN friendinfo;
	bool result = false;
	std::string errorinfo = "";

	if (friend_info.userid == friend_info.friendid)
	{
		errorinfo = GBKToUTF8("无法添加自己");
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
		errorinfo = GBKToUTF8("该用户尚未注册");
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
				errorinfo = GBKToUTF8("好友已添加过");
			}
			break;
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = GBKToUTF8("当前用户未注册");
	}

	//int sock = _mapUserOnline[friend_info.friendid];

	//if (!SendDataPack(sock, datapack))
	//	errorinfo = "信息发送failed";
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
		PrintfInfo(0x8, "log---Add friend successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Add friend failed error info:%s", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandDelFriendReturn(int socket, DATA_PACK *datapack)
{
	FRIEND_INFO friendinfo;
	memcpy(&friendinfo, datapack->data, sizeof(FRIEND_INFO));
	PrintfInfo(0xd, "data---Acconunt:%d", friendinfo.userid);
	PrintfInfo(0xd, "data---Friend:%d", friendinfo.friendid);
	PrintfInfo(0xd, "data---Info:%s", friendinfo.info);

	/*校验好友信息*/
	int usersize = 0;
	bool result = false;
	std::string errorinfo = "";

	if (!_mapUserOnline.count(friendinfo.userid))
	{
		errorinfo = GBKToUTF8("用户未登录");
		goto LLL;
	}

	if (friendinfo.userid == friendinfo.friendid)
	{
		errorinfo = GBKToUTF8("无法删除自己");
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
				errorinfo = GBKToUTF8("你没有该好友");
			}
			break;
		}
	}
	if (errorinfo.empty() && !result)
	{
		errorinfo = GBKToUTF8("当前用户尚未注册");
	}

LLL:
	/*打包结果*/
	int datasize = 0;
	void* data = nullptr;
	if (result)
	{
		data = nullptr;
		datasize = 0;
		PrintfInfo(0x8, "log---Del friend successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Del friend failed error info:%s", data);
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
		PrintfInfo(0xd, "data---Sender:%d", chatinfo.useridfrom);
		PrintfInfo(0xd, "data---Receiver:%d", chatinfo.useridto);
		PrintfInfo(0xd, "data---Message:%s", chatinfo.info);

		userid = chatinfo.useridfrom;
		friendid = chatinfo.useridto;
		info = chatinfo.info;
	}
	else if (datapack->datatype == CHAT_FILE)
	{
		OSSFILEINFO fileinfo;
		memcpy(&fileinfo, datapack->data, sizeof(fileinfo));
		PrintfInfo(0xd, "data---Sender:%d", fileinfo.useridfrom);
		PrintfInfo(0xd, "data---Receiver:%d", fileinfo.useridto);
		PrintfInfo(0xd, "data---bucket:%s", fileinfo.bucket);
		PrintfInfo(0xd, "data---object:%s", fileinfo.object);

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
				errorinfo = GBKToUTF8("你还不是对方的好友");
			}
			friendexit = true;
			break;
		}
	}
	if (!friendexit)
	{
		errorinfo = GBKToUTF8("对方已经注销账号");
		goto LLL;
	}

	if (!_mapUserOnline.count(friendid))
	{
		errorinfo = GBKToUTF8("好友未登录,信息将缓存，在好友上线后发送");

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
			errorinfo = GBKToUTF8("信息发送失败");
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
		PrintfInfo(0x8, "log---Send chatinfo successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Send chatinfo failed error info:%s", data);
	}

	SendDataPackReturn(socket, (COMMANDTYPE)datapack->commandtype, RESULT(result), data, datasize);
}

void CMSServer::CommandFriendInfoReturn(int socket, DATA_PACK *datapack)
{
	USER_INFO userinfo;
	memcpy(&userinfo, datapack->data, sizeof(USER_INFO));
	PrintfInfo(0xd, "data---Account:%d", userinfo.userid);
	PrintfInfo(0xd, "data---Password:%s", userinfo.password);

	int usersize = 0;
	bool result = false;
	std::string errorinfo = "";
	if (!_mapUserOnline.count(userinfo.userid))
	{
		errorinfo = GBKToUTF8("用户未登录");
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
		PrintfInfo(0x8, "log---Get frined info successfully");
	}
	else
	{
		data = (void*)errorinfo.c_str();
		datasize = errorinfo.length();
		PrintfInfo(0x8, "log---Get fiend infofailed error info:%s", data);
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
	PrintfInfo(0x8, "log---Notify friends to go %s", state == COMMAND_LOGIN ? "online" : "offline");

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