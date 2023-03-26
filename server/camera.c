#include "head.h"
#define WIDTH 640
#define HIGHT 480
extern pic mypic;
extern pthread_mutex_t c_mutex;
void *camera_on(void *arg)
{
	int connfd = (int)arg;
	printf("this is camera module!\n");
	int ret;
	int count = 0;

	//1.打开设备
	int camerafd = open("/dev/video0",O_RDWR);
	if(camerafd < 0)
	{
		perror("open camera failed");
		pthread_exit("open camera failed");
	}

	//2.配置（获取支持格式）ioctl（文件描述符，命令，命令对应的结构体）
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.index=0;
	fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while(ioctl(camerafd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
	{
		printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
		fmtdesc.index++;
	}
	//配置采集格式
	struct v4l2_format format;
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = WIDTH;
	format.fmt.pix.height = HIGHT;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;

	ret = ioctl(camerafd,VIDIOC_S_FMT,&format);
	if(ret < 0)
		perror("格式设置失败");
	memset(&format,0,sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(camerafd,VIDIOC_G_FMT,&format);
	if(ret < 0)
		perror("获取格式失败");
	if(format.fmt.pix.width == WIDTH && format.fmt.pix.height == HIGHT && format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
		printf("格式设置成功\n");

	//3.申请内核缓冲队列
	struct v4l2_requestbuffers reqbuffer;
	reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuffer.count = 4;//申请4个队列缓冲区
	reqbuffer.memory = V4L2_MEMORY_MMAP;//映射方式

	ret = ioctl(camerafd,VIDIOC_REQBUFS,&reqbuffer);//VIDIOC_REQBUFS分配内存
	if(ret < 0)
		perror("申请失败");
	printf("申请成功\n");

	//4.把内核缓冲队列映射到用户空间
	unsigned char *mapptr[4];//保存映射后用户空间的首地址
	unsigned int mapsize[4];  //用于存放映射的长度，便于后期释放映射，
	struct v4l2_buffer mapbuffer;
	mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int i;
	for(i=0;i<reqbuffer.count;i++)//从内核空间中查询一个空间做映射
	{
		mapbuffer.index = i;
		if(ioctl(camerafd,VIDIOC_QUERYBUF,&mapbuffer) < 0)
			perror("查询内核空间队列失败");
		//PROT_READ 页内容可以被读取
		//PROT_WRITE 页可以被写入
		//成功执行时，mmap()返回被映射区的 指针
		mapptr[i] = (unsigned char *)mmap(NULL,mapbuffer.length,PROT_READ|PROT_WRITE,MAP_SHARED,camerafd,mapbuffer.m.offset);
		mapsize[i] = mapbuffer.length;//保存每次映射的长度

		//通知，使用完毕放回去
		if(ioctl(camerafd,VIDIOC_QBUF,&mapbuffer) < 0)
			perror("放回失败");
	}
	printf("映射成功\n");

	//5.开始采集
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//VIDIOC_STREAMON命令的参数type为int型
	ret = ioctl(camerafd,VIDIOC_STREAMON,&type);
	if(ret < 0)
	{
		perror("数据采集开启失败");
	}
	//从队列中提取一帧数据
	struct v4l2_buffer readbuffer;
	char cmd_buf[32] = {0};
	int len = 0;
	char sizebuf[16] = {0};
	printf("-----------已成功进入摄入模块---------\n");
	/***************************出队--发送图片--入队**********************/
	while(1)
	{
		memset(&readbuffer,0,sizeof(readbuffer));

		if(read(connfd,cmd_buf,32) <= 0)
		{
			perror("receive camera cmd fail");
			pthread_exit("ss");
		}	
		readbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		readbuffer.memory = V4L2_MEMORY_MMAP;
		//出队一张图片
		printf("this is camera pop\n");
		ret = ioctl(camerafd,VIDIOC_DQBUF,&readbuffer);
		if(ret < 0)
		{
			perror("数据提取失败");
		}
		//mapptr[mapbuffer.index]进行索引，找到队列中的数据
		/***********************发送图片**********************/
		len = 0;
		sprintf(sizebuf,"%d",readbuffer.bytesused);
		printf("图片大小psize=%s\n",sizebuf);
		if(write(connfd,sizebuf,sizeof(sizebuf))<0)//发送图片大小
			perror("write");
		while(len < readbuffer.bytesused )//发送图片内容
		{
			ret = write(connfd, mapptr[readbuffer.index]+len, readbuffer.bytesused -len );
			len += ret;
			printf("已发送图片字节数=%d\n",len);
		}
		/**********************发送完毕***********************/
		//入队一张图片
		ret = ioctl(camerafd,VIDIOC_QBUF,&readbuffer);
		if(ret < 0)
		{
			perror("队列放回失败");
		}
	}
	//关闭采集
	ret = ioctl(camerafd,VIDIOC_STREAMOFF,&type);
	if(ret < 0)
	{
		perror("关闭采集失败");
	}
	//取消映射
	for(i=0;i<reqbuffer.count;i++)
	{
		munmap(mapptr[i],mapsize[i]);
	}
	//关闭设备
	close(camerafd);
	printf("视频设备关闭成功！\n");
	return 0;
}
