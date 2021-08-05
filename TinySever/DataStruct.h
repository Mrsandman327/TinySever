#define BANDWIDTH  1 //����
enum type
{
	USERTEXT = 0,				//���죬����
	USERFILE,					//�����ļ�
	FILEBACK,					//�ļ�����Ӧ��
	OSSFILE						//�ϴ���OSS���ļ�֪ͨ
};
#pragma pack(push,1)
//�����Ǵ���ֻ��1M��1024byte�������Ըýṹ������ʹ��1024�ֽڣ���ֹ��Ƭ
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
	COMMAND_SIGIN, /*ע��*/
	COMMAND_SIGOUT, /*ע��*/
	COMMAND_LOGIN,/*����*/
	COMMAND_LOGOUT,/*�ǳ�*/
	COMMAND_SINGLECHAT,/*����*/
	COMMAND_GROUPCHAT/*Ⱥ��*/
};

enum DATATYPE
{
	CHAT_TEXT,/*��������*/
	CHAT_FILE/*�����ļ�*/
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
	char commandtype;/*�������� 1byte*/
	char datatype;/*�������� 1byte*/
	char data[1022];/*���� 1022byte*/
};

#pragma pack(pop)