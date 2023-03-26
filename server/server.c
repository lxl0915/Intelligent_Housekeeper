#include "head.h"	
#include "serial.h"
//创建锁变量
pthread_mutex_t c_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
pic mypic;//获取Camera数据结构体变量
int serial_i = 0;
extern data_t mydata;//获取mo数据结构体变量
char serial_buf[10];//给m0发送命令缓冲区
int serial_fd;//定义操作m0串口的文件描述符
void *serial_recv(void *arg);

void *ClientFun_cmd(void *arg);
void *ClientFun_login(void *arg);
void *ClientFun_camera(void *arg);
void *ClientFun_m0(void *arg);

void *server_fun(void *arg);
void data_camera(int connfd);
void *data_m0(void *arg);
void do_register(int acceptfd, MES *mes, sqlite3 *db);
int do_login(int acceptfd, MES *mes, sqlite3 *db);
int main(int argc, const char *argv[])
{
	mypic.pbuf = NULL;
	pthread_t serial_tid,server_tid;
	int ret = 0;
	pthread_t client_cmd,client_cam,client_m0,client_log;//创建线程分别服务不间断获取camera和m0数据，交互命令及登录注册
	//串口初始化
	serial_fd = serial_fun();
	if(serial_fd == -1)
	{
		perror("serial init falied!");
		exit(-1);
	}
	else
	{
		printf("------------------串口初始化成功---------------\n");
	}
	//创建m0开发板子进程
	if((pthread_create(&serial_tid,NULL,serial_recv,(void *)serial_fd))!= 0)
	{
		perror("serial pthread create fail");
		return -1;
	}
	pthread_detach(serial_tid);

	/*tcp并发服务器创建*/

	/*--------------------------------------socket_cmd------------------------*/
	//1.创建套接字cmd
	int sockfd_cmd = socket(AF_INET,SOCK_STREAM,0);//第一个参数表明为IPv4地址，第二个参数表示为流式套接字，即面向TCP的套接字
	if( sockfd_cmd < 0)
	{
		pthread_exit("socket create failed");
	}
	//2.套接字绑定相关信息
	struct sockaddr_in saddr_cmd;//定义网络地址结构体用于存放IP地址及端口号
	memset(&saddr_cmd,0,sizeof(saddr_cmd));//结构体初始置0
	saddr_cmd.sin_family = AF_INET;//表明为IPv4地址
	saddr_cmd.sin_port = htons(10086);//因为不同主机的存储模式（大端小端）可能不同，所以将主机字节序转为网络字节序类似的还有ntohs
	saddr_cmd.sin_addr.s_addr = inet_addr("0.0.0.0");//将点分十进制的IP地址字符串转为无符号整形数据-->sin_addr结构体中的对应类型（32位无符号整形）
	ret = bind(sockfd_cmd,(struct sockaddr*)&saddr_cmd,sizeof(saddr_cmd));//将网络地址结构强制转换为通用地址结构才能供bind函数使用
	if( ret < 0)
	{
		pthread_exit("bind failed");
	}
	//3.设置套接字为监听套接字
	ret = listen(sockfd_cmd,8);//backlog为等待连接队列的长度
	if( ret < 0)
	{
		pthread_exit("listen failed");
	}
	//创建客户端接入处理子线程
	pthread_create(&client_cmd,NULL,ClientFun_cmd,(void *)sockfd_cmd);
	pthread_detach(client_cmd);//设置线程为分离属性,线程结束后自动释放资源

	/*--------------------------------------socket_m0------------------------*/
	//1.创建套接字m0
	int sockfd_m0 = socket(AF_INET,SOCK_STREAM,0);//第一个参数表明为IPv4地址，第二个参数表示为流式套接字，即面向TCP的套接字
	if( sockfd_m0 < 0)
	{
		pthread_exit("socket create failed");
	}
	//2.套接字绑定相关信息
	struct sockaddr_in saddr_m0;//定义网络地址结构体用于存放IP地址及端口号
	memset(&saddr_m0,0,sizeof(saddr_m0));//结构体初始置0
	saddr_m0.sin_family = AF_INET;//表明为IPv4地址
	saddr_m0.sin_port = htons(10088);//因为不同主机的存储模式（大端小端）可能不同，所以将主机字节序转为网络字节序类似的还有ntohs
	saddr_m0.sin_addr.s_addr = inet_addr("0.0.0.0");//将点分十进制的IP地址字符串转为无符号整形数据-->sin_addr结构体中的对应类型（32位无符号整形）
	ret = bind(sockfd_m0,(struct sockaddr*)&saddr_m0,sizeof(saddr_m0));//将网络地址结构强制转换为通用地址结构才能供bind函数使用
	if( ret < 0)
	{
		pthread_exit("bind failed");
	}
	//3.设置套接字为监听套接字
	ret = listen(sockfd_m0,8);//backlog为等待连接队列的长度
	if( ret < 0)
	{
		pthread_exit("listen failed");
	}
	pthread_create(&client_m0,NULL,ClientFun_m0,(void *)sockfd_m0);
	pthread_detach(client_m0);//设置线程为分离属性,线程结束后自动释放资源

	/*--------------------------------------socket_login------------------------*/
	//1.创建套接字login
	int sockfd_log = socket(AF_INET,SOCK_STREAM,0);//第一个参数表明为IPv4地址，第二个参数表示为流式套接字，即面向TCP的套接字
	if( sockfd_log < 0)
	{
		pthread_exit("socket create failed");
	}
	//2.套接字绑定相关信息
	struct sockaddr_in saddr_log;//定义网络地址结构体用于存放IP地址及端口号
	memset(&saddr_log,0,sizeof(saddr_log));//结构体初始置0
	saddr_log.sin_family = AF_INET;//表明为IPv4地址
	saddr_log.sin_port = htons(10089);//因为不同主机的存储模式（大端小端）可能不同，所以将主机字节序转为网络字节序类似的还有ntohs
	saddr_log.sin_addr.s_addr = inet_addr("0.0.0.0");//将点分十进制的IP地址字符串转为无符号整形数据-->sin_addr结构体中的对应类型（32位无符号整形）
	//设置端口复用
	int opt = 1;
	setsockopt( sockfd_log, SOL_SOCKET,SO_REUSEADDR,(const void *)&opt,sizeof(opt));
	ret = bind(sockfd_log,(struct sockaddr*)&saddr_log,sizeof(saddr_log));//将网络地址结构强制转换为通用地址结构才能供bind函数使用
	if( ret < 0)
	{
		pthread_exit("bind failed");
	}
	//3.设置套接字为监听套接字
	ret = listen(sockfd_log,8);//backlog为等待连接队列的长度
	if( ret < 0)
	{
		pthread_exit("listen failed");
	}
	pthread_create(&client_log,NULL,ClientFun_login,(void *)sockfd_log);
	pthread_detach(client_log);//设置线程为分离属性,线程结束后自动释放资源

	/*--------------------------------------socket_camera------------------------*/
	//1.创建套接字camera
	int sockfd_cam = socket(AF_INET,SOCK_STREAM,0);//第一个参数表明为IPv4地址，第二个参数表示为流式套接字，即面向TCP的套接字
	if( sockfd_cam < 0)
	{
		pthread_exit("socket create failed");
	}
	//2.套接字绑定相关信息
	struct sockaddr_in saddr_cam;//定义网络地址结构体用于存放IP地址及端口号
	memset(&saddr_cam,0,sizeof(saddr_cam));//结构体初始置0
	saddr_cam.sin_family = AF_INET;//表明为IPv4地址
	saddr_cam.sin_port = htons(10087);//因为不同主机的存储模式（大端小端）可能不同，所以将主机字节序转为网络字节序类似的还有ntohs
	saddr_cam.sin_addr.s_addr = inet_addr("0.0.0.0");//将点分十进制的IP地址字符串转为无符号整形数据-->sin_addr结构体中的对应类型（32位无符号整形）
	ret = bind(sockfd_cam,(struct sockaddr*)&saddr_cam,sizeof(saddr_cam));//将网络地址结构强制转换为通用地址结构才能供bind函数使用
	if( ret < 0)
	{
		pthread_exit("bind failed");
	}
	//3.设置套接字为监听套接字
	ret = listen(sockfd_cam,8);//backlog为等待连接队列的长度
	if( ret < 0)
	{
		pthread_exit("listen failed");
	}
	pthread_create(&client_cam,NULL,ClientFun_camera,(void *)sockfd_cam);
	pthread_detach(client_cam);//设置线程为分离属性,线程结束后自动释放资源
	//主线程结束
	while(1)
	{
		sleep(2);
	}
}
//客户端接入处理子线程
void *ClientFun_cmd(void *arg)
{
	int sockfd = (int)arg;
	char cmd[32] = {0};//cmd缓冲区用于接收客户端的命令
	char buf[64] = {0};//buf缓冲区用于暂时的发送数据
	/*循环等待客户端的接入*/
	struct sockaddr_in caddr;//定义网络结构体变量
	int len = sizeof(caddr);//存放网络结构体变量长度
	printf("准备进入接入客户端，处理控制设备指令，下一步执行accept函数\n");
	int connfd = accept(sockfd,(struct sockaddr*)&caddr,&len);
	if( connfd < 0)
	{
		pthread_exit("M0 recv data accept failed");
	}
	printf("------------------thread-cmd------------------\n");
	printf("-----已有客户端连入------\n");
	printf("客户端的IP：%s\n",inet_ntoa(caddr.sin_addr));     
	printf("客户端的端口：%d\n",ntohs(caddr.sin_port));

	int ret;
	/*读取接入客户端的请求*/
	while(1)
	{
		printf("welcome to cmd!\n");
		ret = read(connfd,cmd,sizeof(cmd));
		if( ret<0 )
		{
			//读取接入客户端的请求错误，子线程退出
			perror("receive cmd failed!");
			pthread_exit("client pthread exit!");
		}
		/*解析客户端的命令*/
		if(ret>0)//有命令接收到
		{
			if(strcmp(cmd,"LIGHT_ON")==0)
			{
				/*开灯*/
				strcpy(serial_buf,"LIGHT_ON");
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}
			}
			else if(strcmp(cmd,"LIGHT_OFF")==0)
			{
				/*关灯*/
				strcpy(serial_buf,"LIGHT_OFF");			
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}

			}
			else if(strcmp(cmd,"BUZZ_ON")==0)
			{
				/*开启蜂鸣器*/
				strcpy(serial_buf,"BUZZ_ON");			
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}
			}
			else if(strcmp(cmd,"BUZZ_OFF")==0)
			{
				/*关闭蜂鸣器*/
				strcpy(serial_buf,"BUZZ_OFF");			
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}
			}
			else if(strcmp(cmd,"FAN_ON")==0)
			{
				/*开启风扇*/
				strcpy(serial_buf,"FAN_ON");			
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}
			}
			else if(strcmp(cmd,"FAN_OFF")==0)
			{
				/*关闭风扇*/
				strcpy(serial_buf,"FAN_OFF");			
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}
			}
			else if(strcmp(cmd,"SHU_ON")==0)
			{
				/*打开数码管*/
				strcpy(serial_buf,"SHU_ON");			
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}
			}
			else if(strcmp(cmd,"SHU_OFF")==0)
			{
				/*关闭数码管*/
				strcpy(serial_buf,"SHU_OFF");			
				printf("%s\n",serial_buf);
				if(serial_send(serial_fd,serial_buf) == -1)
				{
					perror("send m0 cmd error!");
					pthread_exit("send m0 cmd");
				}
			}
			else
			{
				/*客户端命令有误，返回信息到客户端*/
				write(connfd,"cmd error!",64);
				printf("cmd error\n");
			}
		}
	}
}
//摄像头处理子线程
void *ClientFun_camera(void *arg)
{
	pthread_t cam_tid;
	char cmd_buf[32] = {0};//接收客户端发来的获取图片命令
	int sockfd_cam = (int)arg;//套接字
	int ret = 0;
	char sizebuf[16] = {0};
	struct sockaddr_in caddr;//定义网络结构体变量
	int len = sizeof(caddr);//存放网络结构体变量长度
	int connfd = accept(sockfd_cam,(struct sockaddr*)&caddr,&len);
	//创建摄像头子进程
	if((pthread_create(&cam_tid,NULL,camera_on,(void *)connfd))!= 0)
	{
		perror("cam pthread create fail");
	}
	pthread_detach(cam_tid);//设置线程为分离属性,线程结束后自动释放资源
}
//m0处理子线程
void *ClientFun_m0(void *arg)
{
	pthread_t m_pid;
	int sockfd = (int)arg;
	int ret;
	/*循环等待客户端的接入*/
	struct sockaddr_in caddr;//定义网络结构体变量
	int len = sizeof(caddr);//存放网络结构体变量长度
	printf("准备连接客户端，发送M0的环境数据，下一步执行accept函数\n");
	int connfd = accept(sockfd,(struct sockaddr*)&caddr,&len);
	if( connfd < 0)
	{
		pthread_exit("M0 send data accept failed");
	}
	printf("------------------thread-m0------------------\n");
	printf("-----已有客户端连入------\n");
	printf("客户端的IP：%s\n",inet_ntoa(caddr.sin_addr));     
	printf("客户端的端口：%d\n",ntohs(caddr.sin_port));
	while(1)
	{
		sleep(1);
		//发送环境信息
		printf("this is send m0 enviroment!\n");
		ret = write(connfd,&mydata,sizeof(mydata));

		printf("this is send m0 enviroment22222222!\n");
		if(ret<0)
		{
			perror("m0 enviroment send fail.\n");
			pthread_exit("enviroment send");
		}
		printf("tmp = %d, hum = %d, light = %d\n",mydata.tmp,mydata.hum,mydata.light);
	}
}
//登录注册处理子线程
void *ClientFun_login(void *arg)
{

	MES mes;//与客户端的通信结构体
	sqlite3 *db;//登录注册时查询使用
	char cbuf[32] = {0};//返回客户端登录注册结果
	int sockfd = (int)arg;
	//打开数据库login.db
	if(sqlite3_open("login.db",&db) != SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));
		pthread_exit("sqlite3 open failed");
	}
	else
	{
		printf("open DATABASE success.\n");
	}

	/*循环等待客户端的接入*/
	struct sockaddr_in caddr;//定义网络结构体变量
	int len = sizeof(caddr);//存放网络结构体变量长度
	printf("准备接入客户端，进行登录注册功能，下一步执行accept函数\n");
	int connfd = accept(sockfd,(struct sockaddr*)&caddr,&len);
	if( connfd < 0)
	{
		pthread_exit("register login connect accept failed");
	}
	printf("------------------thread-login------------------\n");
	printf("-----已有客户端连入------\n");
	printf("客户端的IP：%s\n",inet_ntoa(caddr.sin_addr));     
	printf("客户端的端口：%d\n",ntohs(caddr.sin_port));

	//客户端登录注册
	while(1)
	{
		int ret = read(connfd,&mes,sizeof(mes));
		if(ret<0)
		{
			perror("read mes failed.\n");
			close(connfd);//关闭连接
			pthread_exit("read mes failed");
		}
		printf("mes-->type:   %d\nmes-->usrname:%s\nmes-->passwd: %s\n",mes.type,mes.usrname,mes.passwd);

		switch(mes.type)
		{
		case 0:
			//注册
			do_register(connfd,&mes,db);
			break;
		case 1:
			//登录
			if(do_login(connfd,&mes,db) == 1)
				pthread_exit("login exit");
			break;
		default:
			//命令错误
			strcpy(cbuf,"cmd argument error.");
			write(connfd,cbuf,32);
			close(connfd);//关闭连接
			pthread_exit("login exit");
		}
	}
}
#if 0
//获取摄像头数据并发送到客户端子线程
void data_camera(int connfd)
{

	int ret = 0;
	int len = 0;
	char sizebuf[16] = {0};
	if(pthread_mutex_lock(&c_mutex) != 0)
	{
		perror("send pthread_mutex_unlock");
		exit(EXIT_FAILURE);
	}
	if(mypic.psize == 0)
	{
		printf("mypic.psize == 0!\n");
		sleep(10000);
		return ;
		//continue ;
	}
	len = 0;
	//加锁访问与camera共同访问的结构体变量
	sprintf(sizebuf,"%d",mypic.psize);
	printf("图片大小psize=%s\n",sizebuf);
	if(write(connfd,sizebuf,sizeof(sizebuf))<0)//发送图片大小
		perror("write");
	while(len < mypic.psize )//发送图片内容
	{
		ret = write(connfd, mypic.pbuf+len, mypic.psize -len );
		len += ret;
		printf("已发送图片字节数=%d\n",len);
	}
	//结构体变量置空
	memset(mypic.pbuf,0,mypic.psize);
	mypic.psize = 0;
	//解锁
	if(pthread_mutex_unlock(&c_mutex) != 0)
	{
		perror("send pthread_mutex_lock");
		exit(EXIT_FAILURE);
	}
	//		usleep(10000);
	//	}
}
#endif
/*****************************************************
 *   注册检测函数
 *   无返回值
 *
 * **************************************************/
void do_register(int acceptfd, MES *mes, sqlite3 *db)
{
	char cbuf[32] = {0};//返回客户端登录注册结果
	char *errmsg;//保存错误信息
	char sql[128];//保存sql语句

	sprintf(sql,"insert into usr values('%s', %s);",mes->usrname,mes->passwd);
	printf("%s\n",sql);
	//插入
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		strcpy(cbuf,"username is already exist!");
		//回写返回数据
		if(write(acceptfd,cbuf,32)<0)
		{
			perror("send register data error!\n");
			return ;
		}
		else
		{
			printf("success send register data!\n");
		}
	}
	else
	{
		printf("client register ok.\n");
		strcpy(cbuf,"client register ok!\n");
		//回写返回数据
		if(write(acceptfd,cbuf,32)<0)
		{
			perror("send register data error!\n");
			return ;
		}
		else
		{
			printf("success send register data!\n");
		}
	}
	return ;
}
/***************************************************
 *    登录检测函数
 *    返回值：-1 查询出现错误
 *    		   1 查询操作成功且有此用户
 *   
 * **************************************************/
int do_login(int acceptfd, MES *mes, sqlite3 *db)
{
	char cbuf[32] = {0};//返回客户端登录注册结果
	char *errmsg;
	char sql[128] = {0};
	int nrow;
	int ncloumn;
	char **resultp;

	sprintf(sql,"select * from usr where usrname = '%s' and passwd = '%s'",mes->usrname,mes->passwd);
	printf("%s\n",sql);
	//查询
	if(sqlite3_get_table(db,sql,&resultp,&nrow,&ncloumn,&errmsg) != SQLITE_OK )
	{
		printf("%s\n",errmsg);
		return -1;//查询出现错误
	}
	else
	{
		printf("get_table ok!\n");
	}
	//查询操作成功
	//数据库中有此用户
	if(nrow == 1)
	{
		strcpy(cbuf,"login yes!");
		write(acceptfd,cbuf,32);
		printf("success send login data!\n");
		close(acceptfd);//登录成功关闭套接字
		return 1;
	}
	//数据库中无此用户
	if(nrow == 0)
	{
		strcpy(cbuf,"usrname/passwd wrong!");
		write(acceptfd,cbuf,32);
		printf("success send login data!\n");
	}
	return 0;
}
