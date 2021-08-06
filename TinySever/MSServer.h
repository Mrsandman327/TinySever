#pragma once
#include "MSSocket.h"
#include "DataStruct.h"

class CMSServer :
	public CSocketObservable
{
public:
	CMSServer(CMSSocket *sub);
	~CMSServer();
public:
	std::string GetCurTime();
public:
	CMSSocket* _socket;
	void Update(int socket);
	void RecvDataProcess(int socket);
	/*COMMAND RETURN*/
	void CommandSigin(int socket);
	void CommandLogin(int socket);

};

