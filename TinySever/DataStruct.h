#pragma pack(push,1)

enum COMMANDTYPE
{
	COMMAND_SIGIN, /*ע��*/
	COMMAND_SIGOUT, /*ע��*/
	COMMAND_LOGIN,/*����*/
	COMMAND_LOGOUT,/*�ǳ�*/
	COMMAND_ADDFRIEND,/*��Ӻ���*/
	COMMAND_DELFRIEND,/*ɾ������*/
	COMMAND_ADDGROUND,/*����Ⱥ��*/
	COMMAND_DELGROUND,/*�˳�Ⱥ��*/
	COMMAND_SINGLECHAT,/*����*/
	COMMAND_GROUPCHAT/*Ⱥ��*/
};

enum DATATYPE
{
	CHAT_TEXT,/*��������*/
	CHAT_FILE,/*�����ļ�*/
	USER_INFO,/*�û���Ϣ*/
	RESULT_RETURN
};

enum RESULT
{
	FAIL,
	OK
};

/*�û���Ϣ 300*/
struct USERINFO{
	char nickname[20];/*20*/
	unsigned int userid;/*4*/
	char password[20];/*20*/
	char userdescription[256];/*256*/
};

/*�ļ���Ϣ 128*/
struct OSSFILEINFO
{
	char bucket[64];
	char object[64];
};

/*�����Ϣ 129*/
struct RESULTINFO
{
	char result;
	char errorinfo[128];
};

/*������Ϣ 1022*/
struct CHATINFO
{
	unsigned int useridfrom;/*4*/
	unsigned int useridto;/*4*/
	char info[1014];/*1014*/
};

/*1024*/ //�����Ǵ���ֻ��1M��1024byte�������Ըýṹ������ʹ��1024�ֽڣ���ֹ��Ƭ
struct DATAPACK{
	char commandtype;/*�������� 1byte*/
	char datatype;/*�������� 1byte*/
	char data[1022];/*���� 1022byte*/
};

#pragma pack(pop)