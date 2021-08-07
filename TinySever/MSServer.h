#pragma once
#include "MSSocket.h"
#include "DataStruct.h"
#include "CJsonObject.hpp"
#include <map>
#define USERJSONFILE "D:\\GIT\\\TinySever\\\Debug\\userinfo.json"

class CMSServer :
	public CSocketObservable
{
public:
	CMSServer(CMSSocket *sub);
	~CMSServer();	
public:
	CMSSocket* _socket;
	neb::CJsonObject _oJsonuserinfo;
	std::map<unsigned int, int> _mapUserOnline;
	bool MapRomoveByValue(int value);
	bool MapRomoveByKey(unsigned int key);
public:
	std::string GetCurTime();
	void ReadUserInfoJson();
	void SaveUserInfoJson();
public:
	void Update(int socket) override;
public:
	void RecvDataProcess(int socket);
	/*COMMAND RETURN*/
	bool SendDataPackReturn(int socket, DATAPACK *datapack);
	void CommandSiginReturn(int socket, DATAPACK *datapack);
	void CommandSigoutReturn(int socket, DATAPACK *datapack);
	void CommandLoginReturn(int socket, DATAPACK *datapack);
	void CommandLogoutReturn(int socket, DATAPACK *datapack);
	void CommandAddFriendReturn(int socket, DATAPACK *datapack);
	void CommandDelFriendReturn(int socket, DATAPACK *datapack);
	void CommandSingleChatReturn(int socket, DATAPACK *datapack);
};

