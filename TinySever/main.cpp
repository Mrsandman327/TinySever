#include "MSServer.h"
#include <stdio.h>
#include <chrono>
#include <string.h>

#ifdef __linux__
#include <sys/io.h>
#include <unistd.h>
#elif  defined(_WIN32)
#include <io.h>
#include <direct.h>
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("arg %d error!\n", argc);
		return 0;
	}

	printf("argv1:IP Adress: %s\n", argv[1]);
	printf("argv2:Port: %s\n", argv[2]);

	if (atoi(argv[2]) <= 0)
	{
		printf("Port: %s port error!\n", argv[2]);
		return 0;
	}

	/*创建被订阅者*/
	CMSSocket *socket = new CMSSocket;

	/*创建订阅者*/
	CMSServer *server = new CMSServer(socket);

	/*添加订阅者*/
	socket->attach_observable(server);
#if 0
	socket->client_connect(argv[1], atoi(argv[2]));

	getchar();
#else
	/*创建服务器*/
	int sockfd = socket->sever_create(argv[1], atoi(argv[2]));

	/*退出*/
	while (true)
	{
		char s[100];
		scanf("%s", s);
		if (strcmp(s, "exit") == 0)
			break;
	}
	socket->severclose(sockfd);
	socket->clear_observable();
#endif

	return 0;
}