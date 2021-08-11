#pragma once
#include "MSSocket.h"
#include "DataStruct.h"
#include "CJsonObject.hpp"
#include <map>

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
	void Prtinf(unsigned short cr, const char *_format, ...);
public:
	void Update(int socket) override;
public:
	void RecvDataProcess(int socket);
	/*COMMAND RETURN*/
	void CommandSiginReturn(int socket, DATA_PACK *datapack);
	void CommandSigoutReturn(int socket, DATA_PACK *datapack);
	void CommandLoginReturn(int socket, DATA_PACK *datapack);
	void CommandLogoutReturn(int socket, DATA_PACK *datapack);
	void CommandAddFriendReturn(int socket, DATA_PACK *datapack);
	void CommandDelFriendReturn(int socket, DATA_PACK *datapack);
	void CommandSingleChatReturn(int socket, DATA_PACK *datapack);
	void CommandFriendInfoReturn(int socket, DATA_PACK *datapack);
	bool SendDataPack(int socket, DATA_PACK *datapack);
	void SendDataPackReturn(int socket, COMMANDTYPE type, RESULT result, void* data, int size);
	/**/
	void SenCacheInfo(unsigned int userid);
	void SaveCacheInfo(SEVERDATATYPE type, void* data, int size);
};

