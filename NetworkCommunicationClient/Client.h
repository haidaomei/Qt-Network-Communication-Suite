#ifndef CLIENT_H
#define CLIENT_H

#include<QTcpSocket>
#include<QTimer>

class Client:public QObject
{
    Q_OBJECT

private:
    QTcpSocket* NetworkCommunicationClient;
    QTimer* InputTimer;

public:
    Client(QObject* parent=nullptr);
    void ConnectToServer();//现在设想的功能?也就连接到服务器,发消息,下线三个而已,额还有一个收消息给忘了
    void DisconnectFromServer();

private slots:
    void Connected();
    void Disconnected();
    void FailedToConnected();
    void Read();
    void CheckStdin();
};

#endif // CLIENT_H
