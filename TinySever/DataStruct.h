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

/*�����������ṹ������*/
enum SEVERDATATYPE
{
	CHAT_TEXT,/*������Ϣ*/
	CHAT_FILE,/*�ļ���Ϣ*/
	NOTIFY_INFO,/*֪ͨ��Ϣ*/
	RESULT_RETURN/*���������ؽ����Ϣ*/
};

/*�ͻ��˶����ṹ������*/
enum CLIENTRDATATYPE
{
	COMMAND
};

enum RESULT
{
	FAIL,
	OK
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
	unsigned int useridfrom;/*4*/
	unsigned int useridto;/*4*/
	char bucket[64];
	char object[64];
};

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@������������Ϣ@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
//һ���������ݽṹ��
/*�����Ϣ 1022*/
struct RESULTINFO_RETURN
{
	unsigned char result;
	char resultinfo[1018];
};


//�����������ݽṹ��
/*�û���Ϣ 84*/
struct USERINFO_RETURN
{
	char nickname[20];/*20*/
	char userdescription[64];/*64*/
};

/*������Ϣ 88*/
struct FRIENDINFO_RETURN
{
	unsigned int friendid;/*4*/
	char nickname[20];/*20*/
	char userdescription[64];/*64*/
};

/*ȫ��������Ϣ 882*/
struct ALLRIENDINFO_RETURN
{
	unsigned short friendsize:6;/*2*/
	unsigned short isonline:10; 
	unsigned int friendid[10];/*40*/
	char nickname[10][20];/*200*/
	char userdescription[10][64];/*640*/
};

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@�ͻ���������Ϣ@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
/*ע����Ϣ(ע��ʹ��) 148*/
struct SIGIN_INFO
{
	char nickname[20];/*20*/
	unsigned int userid;/*4*/
	char password[20];/*20*/
	char userdescription[64];/*64*/
};

/*�û���Ϣ(���룬�ǳ�����ȡ������Ϣ��ע��) 24*/
struct USER_INFO
{
	unsigned int userid;/*4*/
	char password[20];/*20*/
};

/*������Ϣ(��ɾ����) 24*/
struct FRIEND_INFO
{
	unsigned int userid;/*4*/
	unsigned int friendid;/*4*/
	char info[128];
};

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@���ݰ�@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
/*1024*/ //�����Ǵ���ֻ��1M��1024byte�������Ըýṹ������ʹ��1024�ֽڣ���ֹ��Ƭ
struct DATA_PACK
{
	unsigned char commandtype;/*�������� 1byte*/
	unsigned char datatype;/*�������� 1byte*/
	char data[1022];/*���� 1022byte*/
};

#pragma pack(pop)