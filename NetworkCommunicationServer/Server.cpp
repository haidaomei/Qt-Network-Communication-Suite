#include<iostream>
#include"Server.h"

Server::Server(QObject* parent):QObject(parent)
{
    NetworkCommunicationServer=nullptr;
    ListeningPort=0;
}
void Server::BroadcastOnlineInformation(QTcpSocket* ObjectSocket)//因为上线肯定能找到,所以下不设找不到的处理,这里注意json的配置,别干运行时错误出来了
{
    QString OnlineName;
    for(QHash<QString,QTcpSocket*>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QString name=it.key();//别直接写成key,因为被封装了,下同
        QTcpSocket* socket=it.value();
        if(socket==ObjectSocket)
        {
            OnlineName=name;
            break;
        }
    }
    QString message="User "+OnlineName+" has already gone online(^_^)";//不能直接把QString丢到write流,不然会报错
    for(QHash<QString,QTcpSocket*>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QTcpSocket* TempSocket=*it;
        TempSocket->write(message.toUtf8());
    }
}
void Server::BroadcastOfflineInformation(QTcpSocket* ObjectSocket)//ObjectSocket处理了改变的对象,所以现在可以不用关心该和以上方法内部逻辑,只需改变提示词
{
    QString OfflineName;
    for(QHash<QString,QTcpSocket*>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QString name=it.key();
        QTcpSocket* socket=it.value();
        if(socket==ObjectSocket)
        {
            OfflineName=name;
            break;
        }
    }
    QString message="User "+OfflineName+" has already gone offline(#_<-)";
    for(QHash<QString,QTcpSocket*>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QTcpSocket* TempSocket=*it;
        TempSocket->write(message.toUtf8());
    }
}
void Server::ReportAllClientInformation(QTcpSocket* ObjectSocket)
{
    QString OnlineName;
    for(QHash<QString,QTcpSocket*>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QString name=it.key();
        QTcpSocket* socket=it.value();
        if(socket==ObjectSocket)
        {
            OnlineName=name;
            break;
        }
    }
    QString message="Online Users:";
    for(QHash<QString,QTcpSocket*>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QString TempName=it.key();
        message=message+TempName+"\n";
    }
    ObjectSocket->write(message.toUtf8());
}
bool Server::StartServer(int port)//服务器主逻辑是在这里实现的,后面会在main直接初始化一个Server对象(这是单例模式),然后调用这个方法
{
    StopServer();
    NetworkCommunicationServer=new QTcpServer(this);
    ListeningPort=port;
    connect(NetworkCommunicationServer,&QTcpServer::newConnection,this,&Server::NewClientConnected);
    if(!NetworkCommunicationServer->listen(QHostAddress::Any,port))
    {
        std::cout<<"Launching server failed:"<<NetworkCommunicationServer->errorString().toStdString()<<std::endl;
        delete NetworkCommunicationServer;
        NetworkCommunicationServer=nullptr;
        return 0;
    }
    std::cout<<"Launching server succeed,listening port:"<<port<<"(^_^;)"<<std::endl;
    return 1;
}
void Server::StopServer()
{
    if(NetworkCommunicationServer&&NetworkCommunicationServer->isListening())
    {
        NetworkCommunicationServer->close();
        std::cout<<"Server has already stopped listening new connections(^_^)/~~"<<std::endl;
    }
    QList<QTcpSocket*> clients;
    for(auto it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        clients.append(it.value());//为什么要把散列表搞到一个列表里?因为迭代器循环内不能随意修改容器,这点在刚刚做完的数据结构大作业也涉及到了,丢到列表内清除是一个解决方法,当然这里似乎多余,具体细看逻辑
    }
    QString ShutdownMsg="Server is now shutting down(-.-)Zzz";
    for(QTcpSocket* client:clients)
    {
        if(client&&client->state()==QTcpSocket::ConnectedState)
        {
            client->write(ShutdownMsg.toUtf8());
            client->flush();
        }
    }
    for(QTcpSocket* client:clients)
    {
        if(client)
        {
            client->disconnectFromHost();
            client->deleteLater();
        }
    }
    NameToSocketTable.clear();
    SocketToNameTable.clear();
    if(NetworkCommunicationServer)
    {
        delete NetworkCommunicationServer;
        NetworkCommunicationServer=nullptr;//把指针设为nullptr永远是一个好习惯,刚刚结束的数据结构作业就有不设成nullptr导致野指针的问题
    }
    ListeningPort=0;
    std::cout<<"Server has completely stopped(_ _)"<<std::endl;
}
void Server::NewClientConnected()//也可以直接把broadcast逻辑写这里,但是考虑到后续还会把逻辑给到qwidget或者qml,暴露尽可能多的接口是必要的
{
    QTcpSocket* NewClientSocket=NetworkCommunicationServer->nextPendingConnection();
    QString IpAddress=NewClientSocket->peerAddress().toString();
    std::cout<<"New user connected(-_^)Ip:"<<IpAddress.toStdString()<<std::endl;
    NewClientSocket->write("Please enter your name(^_~)Format:User_yourname");
    QString TempName="User"+IpAddress;
    NameToSocketTable.insert(TempName,NewClientSocket);
    SocketToNameTable.insert(NewClientSocket,TempName);
    connect(NewClientSocket,&QTcpSocket::readyRead,this,&Server::ReceiveClientInformation);
    connect(NewClientSocket,&QTcpSocket::disconnected,this,&Server::ClientDisconnected);//这不能带括号,这不是函数这是信号
}
void Server::ClientDisconnected()
{
    QTcpSocket* DisconnectedSocket=qobject_cast<QTcpSocket*>(sender());
    if(!DisconnectedSocket)
    {
        return;
    }
    QString UserName=SocketToNameTable.value(DisconnectedSocket);
    BroadcastOfflineInformation(DisconnectedSocket);
    if(!UserName.isEmpty())
    {
        NameToSocketTable.remove(UserName);
        SocketToNameTable.remove(DisconnectedSocket);
    }
    DisconnectedSocket->deleteLater();
}
void Server::ReceiveClientInformation()
{
    QTcpSocket* ClientSocket=qobject_cast<QTcpSocket*>(sender());
    if(!ClientSocket)
    {
        std::cout<<"Cannot get the socket which sending message(T_T)"<<std::endl;
        return;
    }
    QByteArray data=ClientSocket->readAll();
    if(data.isEmpty())
    {
        return;
    }
    QString message=QString::fromUtf8(data).trimmed();//trimmed()是去掉首尾空白字符的方法,这是处理文本消息的标准三步
    QString CurrentName=SocketToNameTable.value(ClientSocket);
    if(CurrentName.startsWith("User_"))
    {
        if(message.isEmpty())
        {
            ClientSocket->write("Username cannot be empty(._.)");
            return;
        }
        NameToSocketTable.remove(CurrentName);
        NameToSocketTable.insert(message,ClientSocket);
        SocketToNameTable.insert(ClientSocket,message);
        std::cout<<"User register successfully:"<<message.toStdString()<<"^-^"<<std::endl;
        QString SuccessMsg="Register successfully(^^)";
        ClientSocket->write(SuccessMsg.toUtf8());
        BroadcastOnlineInformation(ClientSocket);
    }
    else
    {
        QString ChatMsg=QString("%1:%2").arg(CurrentName,message);
        for(QHash<QString,QTcpSocket*>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
        {
            if(it.value()!=ClientSocket)
            {
                it.value()->write(ChatMsg.toUtf8());
            }
        }
    }
}
