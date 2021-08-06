#include <stdio.h>
#include "MSServer.h"
#include <chrono>

int main(int argc, char *argv[])
{
	printf("argv1:IP Adress: %s\n", argv[1]);
	printf("argv2:Port: %s\n", argv[2]);

	if (atoi(argv[2]) <= 0)
	{
		printf("Port: %s port error!\n", argv[2]);
		return 0;
	}

	/*������������*/
	CMSSocket *socket = new CMSSocket;

	/*����������*/
	CMSServer *server = new CMSServer(socket);

	/*��Ӷ�����*/
	socket->attach_observable(server);
#if 0
	socket->client_connect(argv[1], atoi(argv[2]));

	getchar();
#else
	/*����������*/
	int s = socket->sever_create(argv[1], atoi(argv[2]));

	/*��ʱ*/
	std::chrono::milliseconds dura(8000);
	std::this_thread::sleep_for(dura);

	///*���Ͳ���*/
	//std::string str = "����";1
	//TransInfo transinfo;1
	//transinfo.datalength = str.length();1
	//strcpy(transinfo.filedata, str.c_str());
	//char *buff = new char[sizeof(transinfo)];
	//memset(buff, 0, sizeof(transinfo));
	//memcpy(buff, &transinfo, sizeof(transinfo));
	//if (0 != socket->send_skt(socket->_clientsocklist[0], (char*)buff, sizeof(transinfo)))
	//{
	//	printf("����ʧ��");
	//}
	//delete[] buff;

	
	/*printf("client size:%d\n", (int)socket->getclientsocksize());

	socket->severclose(s);


	printf("client size:%d\n", (int)socket->getclientsocksize());

	socket->clear_observable();*/
	
	getchar();
#endif

	return 0;
}