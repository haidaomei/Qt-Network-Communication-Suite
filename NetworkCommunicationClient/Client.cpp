#include <iostream>
#include <sstream>
#include <QCoreApplication>
#include "Client.h"

// 跨平台非阻塞输入检测
#ifdef _WIN32
#include <conio.h>  // Windows: _kbhit()
#else
#include <sys/select.h>
#include <unistd.h> // Linux/Unix: select()
#endif

Client::Client(QObject* parent) : QObject(parent)
{
    NetworkCommunicationClient = new QTcpSocket(this);
    connect(NetworkCommunicationClient, &QTcpSocket::readyRead, this, &Client::Read);
    connect(NetworkCommunicationClient, &QTcpSocket::connected, this, &Client::Connected);
    connect(NetworkCommunicationClient, &QTcpSocket::errorOccurred, this, &Client::FailedToConnected);
    connect(NetworkCommunicationClient, &QTcpSocket::disconnected, this, &Client::Disconnected);

    InputTimer = new QTimer(this);
    connect(InputTimer, &QTimer::timeout, this, &Client::CheckStdin);
    InputTimer->start(50); // 50ms检测一次
}

void Client::ConnectToServer()
{
    DisconnectFromServer();
    std::cout << "Please enter the ip and port of server(Format:ip port):" << std::endl;
    // 统一使用 std::getline 读取整行，避免残留换行符
    std::string line;
    std::getline(std::cin, line);
    std::istringstream iss(line);
    std::string ipStr;
    int port;
    if (iss >> ipStr >> port) {
        if (port > 0 && port < 65536) {
            QString QIpAddress = QString::fromStdString(ipStr);
            NetworkCommunicationClient->connectToHost(QIpAddress, port);
        } else {
            std::cout << "Invalid port,process exit" << std::endl;
            QCoreApplication::quit();
        }
    } else {
        std::cout << "Invalid format,process exit" << std::endl;
        QCoreApplication::quit();
    }
}

void Client::DisconnectFromServer()
{
    NetworkCommunicationClient->disconnectFromHost();
}

void Client::Connected()
{
    std::cout << "Connect successfully" << std::endl;
}

void Client::Disconnected()
{
    std::cout << "Disconnect successfully" << std::endl;
}

void Client::FailedToConnected()
{
    std::cout << "Connection failed" << std::endl;
}

void Client::Read()
{
    QByteArray data = NetworkCommunicationClient->readAll();
    if (!data.isEmpty()) {
        std::string message = QString::fromUtf8(data).toStdString();
        std::cout << "Receive message:" << message << std::endl;
    }
}

// 跨平台非阻塞输入检测函数
static bool hasInput()
{
#ifdef _WIN32
    return _kbhit() != 0;  // Windows: 检测键盘输入
#else
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);      // 监视标准输入 (fd=0)
    select(1, &fds, nullptr, nullptr, &tv);
    return FD_ISSET(0, &fds);
#endif
}

void Client::CheckStdin()
{
    // 仅当已连接时才处理用户输入
    if (NetworkCommunicationClient->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    // 清除流错误状态（如果有）
    if (std::cin.fail()) {
        std::cin.clear();
    }

    if (hasInput()) {
        std::string line;
        std::getline(std::cin, line);
        if (!line.empty()) {
            NetworkCommunicationClient->write(QString::fromStdString(line).toUtf8());
        }
    }
}
