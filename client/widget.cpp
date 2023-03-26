#include "widget.h"

pic mypic;
char sizebuf[16] = {0};
bool pic_flag = true;
bool m0_flag = true;
Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    this->setWindowTitle("AI智能管家系统");
    timer = new QTimer(this);
    socket = new QTcpSocket;
    socket_cam = new QTcpSocket;
    socket_m0 = new QTcpSocket;
    QPushButton *bt1 = new QPushButton("灯光");
    QPushButton *bt2 = new QPushButton("风扇");
    QPushButton *bt3 = new QPushButton("数码管");
    QPushButton *bt4 = new QPushButton("蜂鸣器");
    QPushButton *bt5 = new QPushButton("摄像头");
    QPushButton *bt6 = new QPushButton("m0");
    QPushButton *bt7 = new QPushButton("语音");
    bt7->setIcon(QIcon(":/icon/mic1.png"));
    lab_cmd = new QLabel();
    QMovie *movie = new QMovie(":/icon/video_ground.jpg");
    label_tmp = new QLabel;
    label_hum = new QLabel;
    label_light = new QLabel;
    label_tmp->setText("温度:");
    label_hum->setText("湿度:");
    label_light->setText("光照:");
    lcd_tmp = new QLCDNumber;
    lcd_tmp->setDecMode();
    lcd_tmp->setSegmentStyle(QLCDNumber::Flat);
    lcd_tmp->setStyleSheet("border:1px solid black; color:black; background: silver;");
    lcd_hum = new QLCDNumber;
    lcd_hum->setDecMode();
    lcd_hum->setSegmentStyle(QLCDNumber::Flat);
    lcd_hum->setStyleSheet("border:1px solid black; color:black; background: silver;");
    lcd_light = new QLCDNumber;
    lcd_light->setSegmentStyle(QLCDNumber::Flat);
    lcd_light->setDecMode();
    lcd_light->setStyleSheet("border:1px solid black; color:black; background: silver;");
    //label = new QLabel();
    //label->setText("温度：   湿度：   光照：");

    QHBoxLayout *hbt = new QHBoxLayout;
    QHBoxLayout *hbt1 = new QHBoxLayout;
    QHBoxLayout *hbt2 = new QHBoxLayout;
    QHBoxLayout *hbt_envir = new QHBoxLayout;
    QVBoxLayout *vbt = new QVBoxLayout;

    hbt->addWidget(bt1);
    hbt->addWidget(bt2);
    hbt->addWidget(bt3);
    hbt->addWidget(bt4);

    hbt2->addWidget(bt5);
    hbt2->addWidget(bt6);

    hbt_envir->addWidget(label_tmp);
    hbt_envir->addWidget(lcd_tmp);
    hbt_envir->addWidget(label_hum);
    hbt_envir->addWidget(lcd_hum);
    hbt_envir->addWidget(label_light);
    hbt_envir->addWidget(lcd_light);

    vbt->addWidget(lab_cmd);
    vbt->addLayout(hbt);
    vbt->addLayout(hbt1);
    vbt->addLayout(hbt2);
    vbt->addLayout(hbt_envir);
    vbt->addWidget(bt7);

    this->setLayout(hbt);
    this->setLayout(vbt);
    lab_cmd->setMovie(movie);
    movie->start();
    movie->stop();
    manger = new QNetworkAccessManager(this);

    connect(timer, SIGNAL(timeout()), this, SLOT(processOneThing()));
    connect(bt1,SIGNAL(clicked(bool)),this,SLOT(bt1_clicked()));
    connect(bt2,SIGNAL(clicked(bool)),this,SLOT(bt2_clicked()));
    connect(bt3,SIGNAL(clicked(bool)),this,SLOT(bt3_clicked()));
    connect(bt4,SIGNAL(clicked(bool)),this,SLOT(bt4_clicked()));
    connect(bt5,SIGNAL(clicked(bool)),this,SLOT(bt5_cam_clicked()));
    connect(bt6,SIGNAL(clicked(bool)),this,SLOT(bt6_cmd_clicked()));
    connect(socket_m0,SIGNAL(readyRead()),this,SLOT(socket_recv()));
    connect(socket_cam,SIGNAL(readyRead()),this,SLOT(socket_cam_recv()));
    connect(bt7,SIGNAL(pressed()),this,SLOT(bt7_pressed()));
    connect(bt7,SIGNAL(released()),this,SLOT(bt7_released()));
    connect(manger,SIGNAL(finished(QNetworkReply*)),this,SLOT(http_recv(QNetworkReply*)));

    socket->connectToHost("192.168.137.130",10086);

    //QByteArray data = "GET_SERIAL";
   // socket->write(data.data(),32);
    //记录灯、风扇、数码管、蜂鸣器的状态0为关，1为开
    flag1 = 0;
    flag2 = 0;
    flag3 = 0;
    flag4 = 0;
}

void Widget::bt1_clicked()//控制灯
{
    if(flag1 == 0){
        QByteArray data = "LIGHT_ON";
        send_data(data);
        qDebug()<<"LIGHT_ON"<<endl;
        flag1 = 1;
    }else{
        QByteArray data = "LIGHT_OFF";
        send_data(data);
        qDebug()<<"LIGHT_OFF"<<endl;
        flag1 = 0;
    }
}
void Widget::bt2_clicked()//控制风扇
{
    if(flag2 == 0){
        QByteArray data = "FAN_ON";
        send_data(data);
        qDebug()<<"FAN_ON"<<endl;
        flag2 = 1;
    }else{
        QByteArray data = "FAN_OFF";
        send_data(data);
        qDebug()<<"FAN_OFF"<<endl;
        flag2 = 0;
    }
}
void Widget::bt3_clicked()//控制数码管
{
    if(flag3 == 0){
        QByteArray data = "SHU_ON";
        send_data(data);
        qDebug()<<"SHU_ON";
        flag3 = 1;
    }else{
        QByteArray data = "SHU_OFF";
        send_data(data);
        qDebug()<<"SHU_OFF";
        flag3 = 0;
    }
}
void Widget::bt4_clicked()//控制蜂鸣器
{
    if(flag4 == 0){
        QByteArray data = "BUZZ_ON";
        send_data(data);
        qDebug()<<"BUZZ_ON"<<endl;
        flag4 = 1;
    }else{
        QByteArray data = "BUZZ_OFF";
        send_data(data);
        qDebug()<<"BUZZ_OFF"<<endl;
        flag4 = 0;
    }
}
void Widget::bt5_cam_clicked()//控制摄像头
{
    socket_cam->connectToHost("192.168.137.130",10087);
    QByteArray data = "GET_CAMERA";    
    socket_cam->write(data.data(),32);
    qDebug()<<"GET_CAMERA"<<endl;
    timer->start(100);
}
void Widget::bt6_cmd_clicked()//申请得到环境数据
{
    if(m0_flag)
    {
     socket_m0->connectToHost("192.168.137.130",10088);
     m0_flag = false;
    }
    QByteArray data = "GET_SERIAL";
    socket_m0->write(data.data(),32);
    qDebug()<<"GET_SERIAL"<<endl;
}
void Widget::send_data(QByteArray data)//传输指令
{
    socket->write(data.data(),32);
}
void Widget::socket_recv()
{
    env_data mydata;
//    qDebug()<<"555555555555555555555555"<<endl;
    socket_m0->read((char*)&mydata,sizeof(mydata));//接收M0数据
//    qDebug()<<"666666666666666666666"<<endl;
//    QString str = "温度："+QString::number(mydata.tmp)+"  "+"湿度："+QString::number(mydata.hum)+"  "+"光照："+QString::number(mydata.light);
//    label->setText(str);
    lcd_tmp->display(mydata.tmp);
    lcd_hum->display(mydata.hum);
    lcd_light->display(mydata.light);
}
void Widget::socket_cam_recv()//接收服务器发送的摄像头信息
{
    if(pic_flag)
    {
        if(16 > socket_cam->bytesAvailable())
            return;
        /*接收图片的大小*/

        socket_cam->read(sizebuf,16);
        QString str(sizebuf) ;

        mypic.psize = str.toInt();
        qDebug()<<"size="<<mypic.psize;
        pic_flag = false;
    }
    else
    {
        if(mypic.psize > socket_cam->bytesAvailable())
            return;
        /*接收图片内容*/

        socket_cam->read(mypic.pbuf,mypic.psize);
        /*设置播放大小*/
        lab_cmd->setMinimumSize(640,480);
        lab_cmd->setMaximumSize(640,480);

        QPixmap pixmap;
        pixmap.loadFromData((uchar*)mypic.pbuf,mypic.psize,"JPEG");
        lab_cmd->setPixmap(pixmap);
        lab_cmd->setScaledContents(true);//让图片自适应label大小
        /*图片显示*/
        qDebug()<<"11";
        pic_flag = true;

    }
}
void Widget::bt7_pressed()//按下语音按钮开始录音
{
    file1 = new QFile();
    file1->setFileName("D:/QTProject/QTRecord/1.pcm");//设置音频文件的存放路径(路径及音频名)   
    if(!file1->open(QIODevice::WriteOnly|QIODevice::Truncate))//文件以重写的方式打开并判断是否打开成功；
    {
        qDebug()<<"打开失败"<<endl;
        exit(1);
    }
    QAudioFormat format;
    format.setByteOrder(QAudioFormat::LittleEndian);//设置高低位，LittleEndian低位优先
    format.setChannelCount(1);//设置声道数通道，1是平声道（mono），2是立体声（stero）
    format.setCodec("audio/pcm");//设置编码器
    format.setSampleSize(16);//设置样本大小，一般为8bit或16bit
    format.setSampleRate(16000);//设置采样赫兹
    format.setSampleType(QAudioFormat::UnSignedInt);//设置采样类型
    QAudioDeviceInfo device = QAudioDeviceInfo::defaultInputDevice();//构建与音频设备通信
    if(!device.isFormatSupported(format))//判断设备是否支持该格式
    {
        qDebug()<<"不支持";
        format = device.nearestFormat(format);
    }
    input = new QAudioInput(format,this);//创建录音对象
    input->start(file1);//开始录音
    qDebug()<<"开始录音"<<endl;
}
void Widget::bt7_released()//松开语音按钮结束录音
{
    input->stop();
    file1->close();
    qDebug()<<"录音结束"<<endl;
    baidu_ASR();
}
void Widget::baidu_ASR()
{
    qDebug()<<"进入语音识别模块";
    QNetworkRequest request;//QNetworkRequest类：通过一个URL地址发起网络协议请求，也保存网络请求的信息
    request.setUrl(QUrl("http://vop.baidu.com/server_api"));//设置Url地址
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");//设置格式
    //封装百度语音可识别的JSON包
    QString filename = "D:/QTProject/QTRecord/1.pcm";//获取打开文件名
    QFile file(filename);//定义文件操作对象
    if(!file.open(QIODevice::ReadOnly| QIODevice::Truncate)){
        qDebug()<<"打开失败"<<endl;
        exit(1);
    }
    QByteArray data = file.readAll();//保存读取到的文件内容
    qDebug()<<"读取成功"<<endl;
    //构造一个Json数据包
    QJsonObject json;
    json["format"] = "pcm";
    json["rate"] = 16000;
    json["channel"] = 1;
    json["token"] = "24.e64c73f14c7337bcc23f11dfbd0e9dcc.2592000.1605072577.282335-22725044";
    json["dev_pid"] = 1537;//此为固定值1537
    json["cuid"] = "2C-4D-54-3A-37-1D";//用户标识，建议使用本机物理地址
    json["speech"] = QString(data.toBase64());//百度要求将完整的录音数据进行base64编码
    json["len"] = data.size();//录音数据编码前的字节数
    qDebug()<<"成功封装Json包";
    //QJsonObject 转 QByteArray
    QJsonDocument doc(json);
    qDebug()<<"识别1111111";
    QByteArray postdata = doc.toJson();
    qDebug()<<"识别2222222";
    //发送Json数据包给百度语音识别
    manger->post(request,postdata);//post发送方式
    qDebug()<<"成功发送Json包";
}
void Widget::http_recv(QNetworkReply *reply)
{
    qDebug()<<"读取百度Json包";
    //QByteArray alldata = reply->readAll();//读出返回的数据
    //qDebug()<<QString(alldata);
    QByteArray allData = reply->readAll();
    QJsonParseError json_error;
    QJsonDocument jsonDoc(QJsonDocument::fromJson(allData, &json_error));
    if(json_error.error != QJsonParseError::NoError)
    {
       qDebug() << "BaiDu json error!";
       return;
    }
    QJsonObject rootObj = jsonDoc.object();
    //获取转化回来的文本内容
    if(rootObj.contains("err_no"))
    {
       if(0==rootObj.take("err_no").toVariant().toString())
       {
             qDebug()<<"语音识别错误!";
       }
       else
       {
             QJsonArray subArray = rootObj.value("result").toArray();
             for(int i = 0; i< subArray.size(); i++)
             {
                 QString str = subArray.at(i).toString();
                 qDebug()<<str;
                 if(str.contains("开灯")){
                     flag1 = 0;
                     bt1_clicked();
                 }else if(str.contains("关灯")){
                     flag1 = 1;
                     bt1_clicked();
                 }else if(str.contains("开风扇")){
                     flag2 = 0;
                     bt2_clicked();
                 }else if(str.contains("关风扇")){
                     flag2 = 1;
                     bt2_clicked();
                 }else if(str.contains("开数码管")){
                     flag3 = 0;
                     bt3_clicked();
                 }else if(str.contains("关数码管")){
                     flag3 = 1;
                     bt3_clicked();
                 }else if(str.contains("开蜂鸣器")){
                     flag4 = 0;
                     bt4_clicked();
                 }else if(str.contains("关蜂鸣器")){
                     flag4 = 1;
                     bt4_clicked();
                 }
             }
        }
    }

}
void Widget::processOneThing()
{
    qDebug()<<"视频1111111"<<endl;
    QByteArray data = "GET_CAMERA";
    socket_cam->write(data.data(),32);
    qDebug()<<"视频2222222"<<endl;

}
Widget::~Widget()
{

}
