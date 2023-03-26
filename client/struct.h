#ifndef STRUCT_H
#define STRUCT_H

#include <QMetaType>

struct message{
public:
    int type;//0：注册  1：登录
    char usrname[32];//用户名
    char passwd[32];//密码
};

struct env_data//m0交互结构体
{
    short int tmp;
    short int hum;
    int light;
};

typedef struct picture//camera交互结构体
{
   int psize;//图片大小
   char pbuf[1024*1024*2];
}pic;
#endif // STRUCT_H
