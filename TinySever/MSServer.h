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
	std::string GetCurTime();
	std::string GetExePath();
	void Prtinf(unsigned short cr, const char *_format, ...);
public:
	/*server socket*/
	CMSSocket* _socket;
	/*server notify*/
	void Update(int socket) override;
	/*online user*/
	neb::CJsonObject _oJsonuserinfo;
	std::map<unsigned int, int> _mapUserOnline;
	bool MapRomoveByValue(int value);
	bool MapRomoveByKey(unsigned int key);
	bool MapInsertData(unsigned int key, int value);
	/*userinfo*/
	void ReadUserInfoJson();
	void SaveUserInfoJson();
	/*cache data*/
	void SenCacheInfo(unsigned int userid);
	void SaveCacheInfo(void* data, int size);
	/*client notify*/
	void LoginStateNotify(int state, unsigned int userid);
	void ServerStateBotify(int state);
	/*send data*/
	bool SendDataPack(int socket, DATA_PACK *datapack);
	void SendDataPackReturn(int socket, COMMANDTYPE type, RESULT result, void* data, int size);
	/*command return*/	
	void RecvDataProcess(int socket);	
	void CommandSiginReturn(int socket, DATA_PACK *datapack);
	void CommandSigoutReturn(int socket, DATA_PACK *datapack);
	void CommandLoginReturn(int socket, DATA_PACK *datapack);
	void CommandLogoutReturn(int socket, DATA_PACK *datapack);
	void CommandAddFriendReturn(int socket, DATA_PACK *datapack);
	void CommandDelFriendReturn(int socket, DATA_PACK *datapack);
	void CommandSingleChatReturn(int socket, DATA_PACK *datapack);
	void CommandFriendInfoReturn(int socket, DATA_PACK *datapack);
	/**/
	void UndefineDataRequst(int socket);
};

