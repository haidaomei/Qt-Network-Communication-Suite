#ifndef SERVER_H
#define SERVER_H

#include<QObject>
#include<QString>
#include<QHash>
#include<QTcpServer>
#include<QTcpSocket>

class Server:public QObject//深入理解qt机制,如果类不继承自qobject,那么会失去信号槽函数connect这些功能
{
    Q_OBJECT//可以简单理解继承publicqobject和这个宏的声明是启用信号槽函数connect机制的语句

private:
    QTcpServer *NetworkCommunicationServer;
    int ListeningPort;

    QHash<QString,QTcpSocket*> NameToSocketTable;
    QHash<QTcpSocket*,QString> SocketToNameTable;//双向,方便查找

public:
    Server(QObject* parent=nullptr);//详见gof《设计模式》组合模式,有别于其他设计模式,比如oop最基本的工厂模式、抽象工厂模式、单例模式,godot的设计模式也是这种(听说unity也是,日后可以开一篇笔记继续讨论)

    void BroadcastOnlineInformation(QTcpSocket* ObjectSocket);//当且仅当每个用户上线广播一次
    void BroadcastOfflineInformation(QTcpSocket* ObjectSocket);//当且仅当一个客户下线广播一次
    void ReportAllClientInformation(QTcpSocket* ObjectSocket);

    bool StartServer(int port);
    void StopServer();//用于开关服务器

private slots://负责处理被connect信号并触发信号后该进行的逻辑
    void NewClientConnected();
    void ClientDisconnected();
    void ReceiveClientInformation();
};

#endif // SERVER_H
