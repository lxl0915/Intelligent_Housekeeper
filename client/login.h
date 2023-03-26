#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include "struct.h"
#include <QDebug>
#include <QTcpSocket>
#include <QByteArray>
#include <QMessageBox>
#include "widget.h"
namespace Ui {
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = 0);
    ~Login();

    void socket_send(struct message *mes);

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();
    void socket_recv();
    void net_connect();
private:
    Ui::Login *ui;
    QTcpSocket *socket;
};

#endif // LOGIN_H
