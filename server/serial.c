#include "serial.h"
extern int serial_i;
data_t mydata;
int serial_open(char* port)
{
	int fd;
	extern int errno;
	fd = open(port,O_RDWR|O_NOCTTY|O_NDELAY);//设置为非阻塞方式
	if(fd < 0)
	{
		printf("open failed,error = %d\n",errno);
		char *msg = strerror(errno);
		printf("Msg: %s\n",msg);
		return -1;
	}
	/*
	 * open方式：
	 * 阻塞：fd = open(port, O_RDWR|NOCTTY)
	 * 非阻塞：fd = open(port,O_RDWR|O_NOCTTY|O_NDELAY);
	 * fcntl函数：
	 * 阻塞：fcntl(fd,F_SETFL,0)
	 * 非阻塞：fcntl(fd,F_SETFL,FNDELAY)
	 */
	if(fcntl(fd, F_SETFL, 0) < 0)	//恢复串口为阻塞状态
	{
		printf("fcntl failed\n");
		return -1;
	}
	if(0 == isatty(STDIN_FILENO))	//判断是否为终端设备
	{
		printf("standard input is not terminal device \n");
		return -1;
	}
	return fd;
}

int serial_init(int myfd,int speed, int flow_ctrl, int databits, int stopbits, int parity)
{
	struct termios options;
	if(tcgetattr(myfd, &options) != 0)	//准备设置参数
	{
		printf("setupSerial failed\n");
		return -1;
	}
	//设置输入和输出波特率
	switch(speed)
	{
		case 115200:
			cfsetispeed(&options, B115200);
			cfsetospeed(&options, B115200);
			break;
		case 19200:
			cfsetispeed(&options, B19200);
			cfsetospeed(&options, B19200);
			break;
		case 9600:
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			break;
		case 4800:
			cfsetispeed(&options, B4800);
			cfsetospeed(&options, B4800);
			break;
		case 2400:
			cfsetispeed(&options, B2400);
			cfsetospeed(&options, B2400);
			break;
		case 1200:
			cfsetispeed(&options, B1200);
			cfsetospeed(&options, B1200);
			break;
		case 300:
			cfsetispeed(&options, B300);
			cfsetospeed(&options, B300);
			break;
		default:
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			break;
	}
	switch(flow_ctrl)
	{
		case 0:
			options.c_cflag &= ~CRTSCTS;
			break;
		case 1:
			options.c_cflag |= CRTSCTS;
			break;
		case 2:
			options.c_cflag |= IXON | IXOFF | IXANY;
			break;
		default:	//默认无流控制
			options.c_cflag &= ~CRTSCTS;
			break;
	}
	options.c_cflag |= CLOCAL;//修改控制模式，保证程序不会占用串口
	options.c_cflag |= CREAD;//修改控制模式，使得能够从串口中读取输入数据
	//设置数据位，屏蔽其他标志位
	options.c_cflag &= ~CSIZE;
	switch(databits)
	{
		case 5:
			options.c_cflag |= CS5;
			break;
		case 6:
			options.c_cflag |= CS6;
			break;
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:	//默认8位数据位
			options.c_cflag |= CS8;
			break;
	}
	//设置校验位
	switch(parity)
	{
		case 'n':
		case 'N'://无奇偶校验位
			options.c_cflag &= ~PARENB;
			options.c_iflag &= ~INPCK;
			break;
		case 'o':
		case 'O'://设置为奇校验
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;
			options.c_cflag |= ISTRIP;
			break;
		case 'e':
		case 'E'://设置为偶校验
			options.c_cflag |= PARENB;
			options.c_cflag &= ~PARODD;
			options.c_cflag |= INPCK;
			options.c_cflag |= ISTRIP;
			break;
		case 's':
		case 'S'://设置为空格
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;
		default:	//默认无校验位
			options.c_cflag &= ~PARENB;
			options.c_iflag &= ~INPCK;
			break;
	}
	switch(stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:	//默认1位停止位
			options.c_cflag &= ~CSTOPB;
			break;
	}
	//修改输出模式，原始数据输出
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	//设置等待时间和最小接收字符
	options.c_cc[VTIME] = 1;	//可以在select中设置
	options.c_cc[VMIN] = 1;
	//如果发生数据溢出，接收数据，但是不再读取  刷新收到的数据但是不读
	tcflush(myfd, TCIFLUSH);
	//激活配置（将修改后的termios数据设置到串口中）
	if(tcsetattr(myfd, TCSANOW, &options) != 0)
	{
		printf("tcsetattr error\n");
		return -1;
	}
	return 0;
}
void *serial_recv(void *arg)
{
	int fd = (int)arg;
	char buf[36];
	int len,fs_sel;
	fd_set fs_read;
	struct timeval time;
	while(serial_i != 0)
	{
		int temp_tmp = 0;		//温度的临时变量
		int temp_hum = 0;		//湿度的临时变量
		int temp_light = 0;		//光照的临时变量

		FD_ZERO(&fs_read);
		FD_SET(fd,&fs_read);
		time.tv_sec = 3;
		time.tv_usec = 0;
		fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
		if(0 < fs_sel)
		{
			int count = 0;
			while(count < 36)
			{
				len = read(fd,buf+count,36-count);
				count += len;
				if(buf[0] != 0xffffffbb)
					count = 0;
			}
		}
		else if(0 == fs_sel)
		{
			printf("time out\n");
			serial_i--;
			if(serial_i == 0)
				break;
			continue;
		}
		else
		{
			perror("select");
			printf("M0 error,please close server,debug M0!\n");
			serial_i = 0;
			break;
		}
		int a1 = (int)buf[5];
		int a2 = (int)buf[7];
		temp_tmp = (short int)((buf[4] << 8) + (a1 &= 0x000000ff));
		temp_hum = (short int)((buf[6] << 8) + (a2 &= 0x000000ff));
		int a3 = (int)buf[20];
		temp_light = (int)((a3 &= 0x000000ff) + (buf[21] << 8) + (buf[22] << 16) + (buf[23] << 24));
		if(temp_light < 0)
		{
			int a1 = (int)buf[5];
			int a2 = (int)buf[7];
			temp_tmp = (short int)((buf[4] << 8) + (a1 &= 0x000000ff));
			temp_hum = (short int)((buf[6] << 8) + (a2 &= 0x000000ff));
			int a3 = (int)buf[19];
			temp_light = (int)((a3 &= 0x000000ff) + (buf[20] << 8) + (buf[21] << 16) + (buf[22] << 24));
		}
		printf("经过含有len的循环! len = %d\n",len);
		mydata.tmp = temp_tmp;
		mydata.hum = temp_hum;
		mydata.light = temp_light;
		printf("%d\n%d\n%d\n",mydata.tmp,mydata.hum,mydata.light);
	}
	if(serial_i == 0)
	{
		printf("Don't receive M0 data,please reboot M0\n");
	}
}

int serial_send(int fd, char *serial_str)
{
	char *buf;
	int  len;
	char led_on[36]  = {0xdd,0x09,0x24,0x00,0x00};
	char led_off[36] = {0xdd,0x09,0x24,0x00,0x01};
	char buzz_on[36] = {0xdd,0x09,0x24,0x00,0x02};
	char buzz_off[36]= {0xdd,0x09,0x24,0x00,0x03};
	char san_on[36]  = {0xdd,0x09,0x24,0x00,0x04};
	char san_off[36] = {0xdd,0x09,0x24,0x00,0x08};
	char shu_on[36]  = {0xdd,0x09,0x24,0x00,0x09};
	char shu_off[36] = {0xdd,0x09,0x24,0x00,0x0a};
	if(strcmp(serial_str,"LIGHT_ON") == 0)
		buf = led_on;
	else if(strcmp(serial_str,"LIGHT_OFF") == 0)
		buf = led_off;
	else if(strcmp(serial_str,"BUZZ_ON") == 0)
		buf = buzz_on;
	else if(strcmp(serial_str,"BUZZ_OFF") == 0)
		buf = buzz_off;
	else if(strcmp(serial_str,"FAN_ON") == 0)
		buf = san_on;
	else if(strcmp(serial_str,"FAN_OFF") == 0)
		buf = san_off;
	else if(strcmp(serial_str,"SHU_ON") == 0)
		buf = shu_on;
	else if(strcmp(serial_str,"SHU_OFF") == 0)
		buf = shu_off;
	else
	{
		printf("please input right order \n");
		return -1;
	}
	len = write(fd,buf,36);
	if(len == 0)
	{
		tcflush(fd,TCOFLUSH);
		perror("write");
		return -1;
	}
	if(len < 0)
	{
		tcflush(fd,TCOFLUSH);//刷新写入的数据但是不传送
		perror("write");
		return -1;
	}
}
int serial_fun()
{	
	int myfd,speed,flow_ctrl,databits,stopbits,parity,ret;
	char *port1;
	serial_i = 5;
	port1 = "/dev/ttyUSB0";
	speed = 115200;
	flow_ctrl = 0;
	databits = 8;
	stopbits = 1;
	parity = 'N';
	myfd = serial_open(port1);
	if(0 > myfd)
	{
		printf("serial_open failed\n");
		return -1;
	}
	ret = serial_init(myfd,speed,flow_ctrl,databits,stopbits,parity);
	if(ret < 0)
	{
		printf("serial_init failed\n");
		return -1;
	}
	return myfd;
}
int serial_close(int myfd)
{
	if(close(myfd) < 0)
		return -1;
	return 0;
}
