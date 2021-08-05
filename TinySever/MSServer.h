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
	CMSSocket* _socket;
	void Update(int socket);
	void RecvDataProcess();

};

