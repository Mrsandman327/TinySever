#define BANDWIDTH  1 //带宽
enum type
{
	USERTEXT = 0,				//聊天，文字
	USERFILE,					//传输文件
	FILEBACK,					//文件传输应答
	OSSFILE						//上传到OSS的文件通知
};
#pragma pack(push,1)
//花生壳带宽只有1M（1024byte），所以该结构体正好使用1024字节，防止分片
struct TransInfo
{
	unsigned char type;//1
	char filename[64];//64
	char filedata[947 + ((BANDWIDTH - 1) * 1024)];
	unsigned int filesize;
	unsigned int dataidx;
	unsigned int datalength;//4

	TransInfo()
	{
		type = USERTEXT;
		memset(filename, 0, sizeof(filename));
		memset(filedata, 0, sizeof(filedata));
		datalength = sizeof(filedata);
		dataidx = 0;
		filesize = 0;
	}
};


enum COMMANDTYPE
{
	COMMAND_SIGIN, /*注册*/
	COMMAND_SIGOUT, /*注销*/
	COMMAND_LOGIN,/*登入*/
	COMMAND_LOGOUT,/*登出*/
	COMMAND_SINGLECHAT,/*单聊*/
	COMMAND_GROUPCHAT/*群聊*/
};

enum DATATYPE
{
	CHAT_TEXT,/*聊天文字*/
	CHAT_FILE/*聊天文件*/
};

/*280*/
struct USERINFO{
	char nickname[20];/*20*/
	unsigned int userid;/*4*/
	char userdescription[256];/*256*/
};

/*128*/
struct OSSFILEINFO
{
	char bucket[64];
	char object[64];
};

/*1024*/
struct DATAPACK{
	char commandtype;/*命令类型 1byte*/
	char datatype;/*数据类型 1byte*/
	char data[1022];/*数据 1022byte*/
};

#pragma pack(pop)