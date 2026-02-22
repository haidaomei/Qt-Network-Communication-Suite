#include<iostream>
#include<QStringList>
#include"Server.h"

Server::Server(QObject* parent):QObject(parent)
{
    NetworkCommunicationServer=nullptr;
    ListeningPort=0;
}
void Server::BroadcastOnlineInformation(QTcpSocket* ObjectSocket)//因为上线肯定能找到,所以下不设找不到的处理,这里注意json的配置,别干运行时错误出来了
{
    QString OnlineName;
    for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QString name=it.key();//别直接写成key,因为被封装了,下同
        QTcpSocket* socket=it->socket;
        if(socket==ObjectSocket)
        {
            OnlineName=name;
            break;
        }
    }
    QString message="User "+OnlineName+" has already gone online";//不能直接把QString丢到write流,不然会报错
    for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QTcpSocket* TempSocket=it->socket;
        if(it->IsRegistered)
        {
            TempSocket->write(message.toUtf8());
        }
    }
}
void Server::BroadcastOfflineInformation(QTcpSocket* ObjectSocket)//ObjectSocket处理了改变的对象,所以现在可以不用关心该和以上方法内部逻辑,只需改变提示词
{
    QString OfflineName;
    for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QString name=it.key();
        QTcpSocket* socket=it->socket;
        if(socket==ObjectSocket)
        {
            OfflineName=name;
            break;
        }
    }
    QString message="User "+OfflineName+" has already gone offline";
    for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        QTcpSocket* TempSocket=it->socket;
        if(it->IsRegistered)
        {
            TempSocket->write(message.toUtf8());
        }
    }
}
void Server::ReportAllClientInformation(QTcpSocket* ObjectSocket)
{
    QString message="Online Users:\n";
    for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
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
        std::cout<<"Launching server failed"<<std::endl;
        delete NetworkCommunicationServer;
        NetworkCommunicationServer=nullptr;
        return 0;
    }
    std::cout<<"Launching server succeed,listening port:"<<port<<std::endl;
    return 1;
}
void Server::StopServer()
{
    if(NetworkCommunicationServer&&NetworkCommunicationServer->isListening())
    {
        NetworkCommunicationServer->close();
        std::cout<<"Server has already stopped listening new connections"<<std::endl;
    }
    QList<QTcpSocket*> clients;
    for(auto it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        clients.append(it->socket);//为什么要把散列表搞到一个列表里?因为迭代器循环内不能随意修改容器,这点在刚刚做完的数据结构大作业也涉及到了,丢到列表内清除是一个解决方法,当然这里似乎多余,具体细看逻辑
    }
    QString ShutdownMsg="Server is now shutting down";
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
    //SocketToNameTable.clear();
    if(NetworkCommunicationServer)
    {
        delete NetworkCommunicationServer;
        NetworkCommunicationServer=nullptr;//把指针设为nullptr永远是一个好习惯,刚刚结束的数据结构作业就有不设成nullptr导致野指针的问题
    }
    ListeningPort=0;
    std::cout<<"Server has completely stopped"<<std::endl;
}
void Server::NewClientConnected()//也可以直接把broadcast逻辑写这里,但是考虑到后续还会把逻辑给到qwidget或者qml,暴露尽可能多的接口是必要的
{//这是信号槽,连接信号在start函数
    QTcpSocket* NewClientSocket=NetworkCommunicationServer->nextPendingConnection();
    QString IpAddress=NewClientSocket->peerAddress().toString();
    std::cout<<"New user connected,Ip:"<<IpAddress.toStdString()<<std::endl;
    QString TempName="User"+IpAddress;
    UserInfo TempInfo;
    TempInfo.IsRegistered=0;
    TempInfo.socket=NewClientSocket;
    NameToSocketTable.insert(TempName,TempInfo);
    //SocketToNameTable.insert(TempInfo,TempName);
    connect(NewClientSocket,&QTcpSocket::readyRead,this,&Server::ReceiveClientInformation);
    connect(NewClientSocket,&QTcpSocket::disconnected,this,&Server::ClientDisconnected);//这不能带括号,这不是函数这是信号
}
void Server::ClientDisconnected()
{
    QTcpSocket* DisconnectedSocket=qobject_cast<QTcpSocket*>(sender());//只有发送信号的socket,信号槽是可以用这种形式的语法获取发送信号的对象的
    QString UserName;
    if(!DisconnectedSocket)
    {
        return;
    }
    for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
    {
        if(it->socket==DisconnectedSocket)
        {
            UserName=it.key();
        }
    }
    BroadcastOfflineInformation(DisconnectedSocket);
    std::cout<<"User "<<UserName.toStdString()<<" has already gone offline"<<std::endl;
    if(!UserName.isEmpty())
    {
        NameToSocketTable.remove(UserName);
        //SocketToNameTable.remove(DisconnectedSocket);反正也暂时用不到直接注释掉
    }
    DisconnectedSocket->deleteLater();
}
/*void Server::ReceiveClientInformation()
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
}这是旧版本的receive逻辑,下面是拓写的更结构化的*/
void Server::ReceiveClientInformation()//仿照mc的聊天方式
{
    QTcpSocket* ClientSocket=qobject_cast<QTcpSocket*>(sender());//现在拥有的唯一标识是发信号的socket,下面根据hash进行操作
    if(!ClientSocket)
    {
        std::cout<<"Fail to Receive a information,cannot get the socket which sending message(T_T)"<<std::endl;
        return;
    }
    QByteArray data=ClientSocket->readAll();
    if(data.isEmpty())
    {
        return;
    }

    if(data.startsWith('/'))
    {
        QStringList parts=QString::fromUtf8(data).split(' ');

        if(parts[0].length()<=1)
        {
            return;//这里只输入了一个斜杠
        }

        if(parts[0]=="/reg")
        {
            for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
            {
                if(it->socket==ClientSocket)//如果clientsocket存在于表中
                {
                    if(it->IsRegistered)//如果clientsocket已注册
                    {
                        ClientSocket->write("You have already registered");
                        return;//剪枝这一块
                    }
                    else//如果未注册,这里可以添加昵称逻辑,但是比较麻烦先搁置
                    {
                        it->IsRegistered=1;
                        ClientSocket->write("Registering succeed");
                        ReportAllClientInformation(ClientSocket);
                        std::cout<<"User "<<it.key().toStdString()<<" registered"<<std::endl;
                        return;
                    }
                }
            }
        }
        else if(parts[0]=="/msg")
        {
            for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)
            {
                if(it->socket==ClientSocket)//这里是一定能找到的,因为一登录就会进行插入,但没有给注册标识,所以不用担心多次显示未注册
                {
                    if(it->IsRegistered)
                    {
                        if(parts.size()<3)
                        {
                            ClientSocket->write("Usage:/msg <username> <message>");
                            return;
                        }
                        QString message=parts[2];//由于分割方法,私聊信息也不能带空格,懒得改了
                        QString CurrentName=it.key();
                        QString ChatMsg=QString("[%1] %2").arg(CurrentName,message);
                        for(QHash<QString,UserInfo>::iterator it2=NameToSocketTable.begin();it2!=NameToSocketTable.end();++it2)
                        {
                            if(it2.key()==parts[1]&&it2->IsRegistered)
                            {
                                it2->socket->write(ChatMsg.toUtf8());
                                std::cout<<it.key().toStdString()<<" say "<<ChatMsg.toStdString()<<" to "<<it2.key().toStdString()<<std::endl;
                                return;
                            }
                        }
                        ClientSocket->write("Object do not exist or have not registered");//这里就不能用else
                        return;
                    }
                    else
                    {
                        ClientSocket->write("You have not registered yet");
                        return;
                    }
                }
            }
        }
        else
        {
            ClientSocket->write("Unknown command,available commands:/reg,/msg <username> <message>");
        }
    }

    else//群发消息
    {
        for(QHash<QString,UserInfo>::iterator it=NameToSocketTable.begin();it!=NameToSocketTable.end();++it)//对于表中所有用户
        {
            if(it->socket==ClientSocket)//对于正在发消息的用户
            {
                if(it->IsRegistered)//如果用户注册
                {
                    QString message=QString::fromUtf8(data).trimmed();//trimmed()是去掉首尾空白字符的方法,这是处理文本消息的标准三步
                    QString CurrentName=it.key();
                    QString ChatMsg=QString("<%1> %2").arg(CurrentName,message);
                    for(QHash<QString,UserInfo>::iterator it2=NameToSocketTable.begin();it2!=NameToSocketTable.end();++it2)
                    {
                        if(it2->socket!=ClientSocket&&it2->IsRegistered)
                        {
                            it2->socket->write(ChatMsg.toUtf8());//下面不能return否则只发给一个人
                        }
                    }
                    std::cout<<it.key().toStdString()<<" say:"<<ChatMsg.toStdString()<<std::endl;
                }
                else//如果用户未注册
                {
                    ClientSocket->write("You have not registered yet");
                    return;
                }
            }
        }
    }
}
