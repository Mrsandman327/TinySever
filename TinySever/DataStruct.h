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

/*服务器二级结构体类型*/
enum SEVERDATATYPE
{
	CHAT_TEXT,/*聊天信息*/
	CHAT_FILE,/*文件信息*/
	NOTIFY_INFO,/*通知信息*/
	RESULT_RETURN/*服务器返回结果信息*/
};

/*客户端二级结构体类型*/
enum CLIENTRDATATYPE
{
	COMMAND
};

enum RESULT
{
	FAIL,
	OK
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
	unsigned int useridfrom;/*4*/
	unsigned int useridto;/*4*/
	char bucket[64];
	char object[64];
};

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@服务器返回信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
//一级返回数据结构体
/*结果信息 1022*/
struct RESULTINFO_RETURN
{
	unsigned char result;
	char resultinfo[1018];
};


//二级返回数据结构体
/*用户信息 84*/
struct USERINFO_RETURN
{
	char nickname[20];/*20*/
	char userdescription[64];/*64*/
};

/*好友信息 88*/
struct FRIENDINFO_RETURN
{
	unsigned int friendid;/*4*/
	char nickname[20];/*20*/
	char userdescription[64];/*64*/
};

/*全部好友信息 882*/
struct ALLRIENDINFO_RETURN
{
	unsigned short friendsize:6;/*2*/
	unsigned short isonline:10; 
	unsigned int friendid[10];/*40*/
	char nickname[10][20];/*200*/
	char userdescription[10][64];/*640*/
};

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@客户端请求信息@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
/*注册信息(注册使用) 148*/
struct SIGIN_INFO
{
	char nickname[20];/*20*/
	unsigned int userid;/*4*/
	char password[20];/*20*/
	char userdescription[64];/*64*/
};

/*用户信息(登入，登出，获取好友信息，注销) 24*/
struct USER_INFO
{
	unsigned int userid;/*4*/
	char password[20];/*20*/
};

/*好友信息(增删好友) 24*/
struct FRIEND_INFO
{
	unsigned int userid;/*4*/
	unsigned int friendid;/*4*/
	char info[128];
};

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@数据包@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
/*1024*/ //花生壳带宽只有1M（1024byte），所以该结构体正好使用1024字节，防止分片
struct DATA_PACK
{
	unsigned char commandtype;/*命令类型 1byte*/
	unsigned char datatype;/*数据类型 1byte*/
	char data[1022];/*数据 1022byte*/
};

#pragma pack(pop)