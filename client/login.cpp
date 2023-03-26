#include "login.h"
#include "ui_login.h"
bool login_flag = false;
Login::Login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    socket = new QTcpSocket;
    connect(socket,SIGNAL(readyRead()),this,SLOT(socket_recv()));
    connect(socket,&QTcpSocket::connected,this,&Login::net_connect);
    ui->lineEdit_2->setEchoMode(QLineEdit::Password);
    this->setWindowTitle("登录界面");
    ui->widget->setAutoFillBackground(true);
    QImage image;
    QPalette pal;
    image.load(":/icon/background.jpg");
    pal.setBrush(ui->widget->backgroundRole(),QBrush(image));
    ui->widget->setPalette(pal);
    ui->widget->setBaseSize(340,330);
    ui->gridLayout->setAlignment(Qt::AlignCenter);
    ui->pushButton->setMaximumWidth(100);
    ui->pushButton_2->setMaximumWidth(100);
}

void Login::net_connect()//判断是否链接成功
{
    qDebug()<<"连接成功";
}

void Login::on_pushButton_clicked()//注册键
{
    //定以注册结构体及赋值
    QByteArray ba = ui->lineEdit->text().toLatin1();
    QByteArray ba2 = ui->lineEdit_2->text().toLatin1();
    char *ptr = ba.data();
    char *ptr2 = ba2.data();
    message *mes = new message;
    mes->type = 0;
    memcpy(mes->usrname,ptr,32);
    memcpy(mes->passwd,ptr2,32);
    socket_send(mes);//传输给服务器

}

void Login::on_pushButton_2_clicked()//登录键
{
    //定以登录结构体及赋值
    //socket->abort();
    QByteArray ba = ui->lineEdit->text().toLatin1();
    QByteArray ba2 = ui->lineEdit_2->text().toLatin1();
    char *ptr = ba.data();
    char *ptr2 = ba2.data();
    message *mes = new message;
    mes->type = 1;
    memcpy(mes->usrname,ptr,32);
    memcpy(mes->passwd,ptr2,32);
    socket_send(mes);
}
void Login::socket_send(struct message *mes)
{
    //socket->abort();
    if(!login_flag)//第一次点击注册或者登陆按钮才连接服务器
    {
    socket->connectToHost("192.168.137.130",10089);
    login_flag = true;
    }
    socket->write((char*)mes,sizeof(*mes));
    delete mes;
}
void Login::socket_recv()
{
    //接收服务器信息
    char cbuf[32] = {0};
    socket->read(cbuf,sizeof(cbuf));
    if(strcmp(cbuf,"username is already exist!") == 0){
       QMessageBox::question(this,"注册","注册失败，已有该用户",QMessageBox::Yes,QMessageBox::Yes);       
    }
    else if(strcmp(cbuf,"client register ok!") == 0){
        int ret = QMessageBox::question(this,"注册","注册成功",QMessageBox::Yes,QMessageBox::Yes);
        socket->close();
        if(ret == QMessageBox::Yes){
            //跳转代码
            Widget *w = new Widget;
            w->show();
            this->close();
        }
    }else if(strcmp(cbuf,"login yes!") == 0){
        int ret = QMessageBox::question(this,"登录","登录成功",QMessageBox::Yes,QMessageBox::Yes);
        socket->close();
        if(ret == QMessageBox::Yes){
            //跳转代码
            Widget *w = new Widget;
            w->show();
            this->close();
        }
    }else if(strcmp(cbuf,"usrname/passwd wrong!") == 0)
        QMessageBox::question(this,"登录","用户名或密码错误，登录失败",QMessageBox::Yes,QMessageBox::Yes);
    else if(strcmp(cbuf,"cmd argument error.") == 0)
        QMessageBox::question(this,"错误","命令参数错误",QMessageBox::Yes,QMessageBox::Yes);
}

Login::~Login()
{
    delete ui;
}
