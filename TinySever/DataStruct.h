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
	COMMAND_GROUPCHAT,/*Ⱥ��*/
	COMMAND_FRIENDINFO/*������Ϣ*/
};

/*�����ṹ������*/
enum DATATYPE
{
	CHAT_TEXT,/*������Ϣ*/
	CHAT_FILE,/*�ļ���Ϣ*/
	USER_INFO,/*�û���Ϣ*/	
	RESULT_RETURN/*�����Ϣ*/
};

enum RESULT
{
	FAIL,
	OK
};

/*�û�������Ϣ 462*/
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

/*������Ϣ 1022*/
struct CHATINFO
{
	unsigned int useridfrom;/*4*/
	unsigned int useridto;/*4*/
	char info[1014];/*1014*/
};

/*�ļ���Ϣ 128*/
struct OSSFILEINFO
{
	char bucket[64];
	char object[64];
};

/*�û���Ϣ 300*/
struct USERINFO{
	char nickname[20];/*20*/
	unsigned int userid;/*4*/
	char password[20];/*20*/
	char userdescription[256];/*256*/
};

/*������Ϣ 481*/
struct FRIENDINFO
{
	unsigned char friendsize;/*1*/
	char nickname[20][20];/*400*/
	unsigned int userid[20];/*80*/
};

/*�����Ϣ 513*/
struct RESULTINFO
{
	unsigned char result;
	char resultinfo[512];
};

/*1024*/ //�����Ǵ���ֻ��1M��1024byte�������Ըýṹ������ʹ��1024�ֽڣ���ֹ��Ƭ
struct DATAPACK{
	unsigned char commandtype;/*�������� 1byte*/
	unsigned char datatype;/*�������� 1byte*/
	char data[1022];/*���� 1022byte*/
};

#pragma pack(pop)