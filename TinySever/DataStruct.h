#pragma pack(push,1)

enum COMMANDTYPE
{
	COMMAND_SIGIN, /*注册*/
	COMMAND_SIGOUT, /*注销*/
	COMMAND_LOGIN,/*登入*/
	COMMAND_LOGOUT,/*登出*/
	COMMAND_ADDFRIEND,/*添加好友*/
	COMMAND_DELFRIEND,/*删除好友*/
	COMMAND_ADDGROUND,/*加入群聊*/
	COMMAND_DELGROUND,/*退出群聊*/
	COMMAND_SINGLECHAT,/*单聊*/
	COMMAND_GROUPCHAT,/*群聊*/
	COMMAND_FRIENDINFO/*好友信息*/
};

/*二级结构体类型*/
enum DATATYPE
{
	CHAT_TEXT,/*聊天信息*/
	CHAT_FILE,/*文件信息*/
	USER_INFO,/*用户信息*/	
	RESULT_RETURN/*结果信息*/
};

enum RESULT
{
	FAIL,
	OK
};

/*用户完整信息 462*/
struct USERINFOALL
{
	char nickname[20];/*20*/
	unsigned int userid;/*4*/
	char password[20];/*20*/
	char userdescription[256];/*256*/
	unsigned char friendsize;/*1*/
	unsigned char groupsize;/*1*/
	unsigned int friendlist[20];/*80*/
	unsigned int grouplist[20];/*80*/
};

/*聊天信息 1022*/
struct CHATINFO
{
	unsigned int useridfrom;/*4*/
	unsigned int useridto;/*4*/
	char info[1014];/*1014*/
};

/*文件信息 128*/
struct OSSFILEINFO
{
	char bucket[64];
	char object[64];
};

/*用户信息 300*/
struct USERINFO{
	char nickname[20];/*20*/
	unsigned int userid;/*4*/
	char password[20];/*20*/
	char userdescription[256];/*256*/
};

/*好友信息 481*/
struct FRIENDINFO
{
	unsigned char friendsize;/*1*/
	char nickname[20][20];/*400*/
	unsigned int userid[20];/*80*/
};

/*结果信息 513*/
struct RESULTINFO
{
	unsigned char result;
	char resultinfo[512];
};

/*1024*/ //花生壳带宽只有1M（1024byte），所以该结构体正好使用1024字节，防止分片
struct DATAPACK{
	unsigned char commandtype;/*命令类型 1byte*/
	unsigned char datatype;/*数据类型 1byte*/
	char data[1022];/*数据 1022byte*/
};

#pragma pack(pop)