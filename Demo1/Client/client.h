// 声明client类
#ifndef  __CLIENT_H__
#define  __CLIENT_H__
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <iostream>
using namespace std;
#include "except.h"

// 客户端
class Client {
public:
	// 构造器
	Client (short port, string const& ip = "127.0.0.1") 
		throw (ClientException);
	// 析构器
	~Client (void);
	// 连接服务器
	void connectServer (void) throw (SocketException);
	// 连接成功后，获取内核分配的本地协议地址
	void getLocalAddr (void) throw (ClientException);
	// 循环监控并进行数据交互
	void loopSelect (void) throw (ClientException);
	// 发送数据
	bool sendData (void) throw (SendException);
	// 接收数据
	bool recvData (void) throw (RecvException);
	// 处理SIGINT信号
//	friend void handle(int signum);
private:
	short m_port;  // 服务器端口号
	string m_ip;   // 服务器IP地址
	int m_sockfd;  // 套接字描述符
	int m_stdinfd; // 标准输入描述符
};
//int Client::m_sockfd = -1;
#endif  // __CLIENT_H__
