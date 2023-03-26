#ifndef _SERVER_H
#define _SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sqlite3.h>
//服务器
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>//套接字相关函数的头文件
#include <arpa/inet.h>//字节序转换头文件，inet_addr头文件
#include <netinet/in.h>//网络地址结构体的头文件
#include <semaphore.h>//锁变量头文件
//camera
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>   
#define PBUFSIZE 1024*1024*2

//定义与m0交互的结构体
#if 0
//环境信息，服务器只读不写，m0只写不读
typedef struct env_data
{
	short int tmp;
	short int hum;
	int light;
}data_t;
//交互命令，服务器只写不读，m0只读不写
typedef struct m0_cmd
{
	char buf[32];
}mcmd;
#endif
//定义camera交互的结构体

//camera只写不读，服务器只读不写
typedef struct picture
{
	int psize;//图片大小
	char *pbuf;
}pic;

//定义与客户端交互的结构体
typedef struct message
{
	int type;//0：注册  1：登录
	char usrname[32];//用户名
	char passwd[32];//密码
}MES;

extern void *camera_on(void *arg);
#endif


