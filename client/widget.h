#ifndef WIDGET_H
#define WIDGET_H

#include <QTimer>
#include <QWidget>
#include <QTcpSocket>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMovie>
#include <QDebug>
#include <QByteArray>
#include <QIcon>
#include <QFile>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QAudioOutput>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLCDNumber>
#include "struct.h"
#include <QPixmap>
#include "login.h"


class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

    void send_data(QByteArray data);
    void baidu_ASR();

public slots:
    void processOneThing();
    void bt1_clicked();
    void bt2_clicked();
    void bt3_clicked();
    void bt4_clicked();
    void bt5_cam_clicked();
    void bt6_cmd_clicked();
    void socket_recv();
    void socket_cam_recv();
    void bt7_pressed();
    void bt7_released();
    void http_recv(QNetworkReply*reply);
private:
    QTimer *timer;
    QTcpSocket *socket;
    QTcpSocket *socket_cam;
    QTcpSocket *socket_m0;
    QFile *file1;
    QAudioInput *input;
    QNetworkAccessManager *manger;
    QLabel *lab_cmd;
    QLabel *label;
    QLabel *label_tmp;
    QLabel *label_hum;
    QLabel *label_light;
    QLCDNumber *lcd_tmp;
    QLCDNumber *lcd_hum;
    QLCDNumber *lcd_light;
    int flag1;
    int flag2;
    int flag3;
    int flag4;
};

#endif // WIDGET_H
