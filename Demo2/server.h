#ifndef  __SERVER_H__
#define  __SERVER_H__
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;
#include "except.h"

// 声明服务器
class Server 
{
public:
	// 构造器
	Server (short port, string const& ip = "")
		throw (ServerException);
	// 析构器
	~Server (void);
	// 初始化服务器
	void initServer (void) throw (SocketException);
    // 启动监听线程
	void start (void) throw (ThreadException);
    // 监听线程过程
    static void* run (void* arg);
    // 监听线程处理
    void* loopListen (void);
private:
	short m_port;    // 服务器端口号
	string m_ip;     // 服务器IP地址
	int m_listenfd;  // 监听套接字
	pthread_t m_tid; // 线程标识
};

// 声明会话线程
class TalkThread 
{
public:
	// 启动会话线程
	void start (void) throw (ThreadException);
private:
	// 会话线程过程
	static void* run (void* arg);
	// 会话线程处理
	void* loopTalk (void);
	pthread_t m_tid;  // 线程标识
	int m_maxfd;      // 可读集合最大有效下表
};

// 声明会话套接字队列
class FdQueue 
{
public:
	// 构造器
	FdQueue (void);
	// 析构器
	~FdQueue (void);
	void push (int connfd);
	// 弹出会话套接字
	void pop (int connfd);
	int getClients (void);
	// 设置客户端数量+1
	void setClients (void);
	// 获取会话套接字集成员
	int getClientfd (int num);
private:
	pthread_mutex_t m_mutex;  // 互斥锁
    int m_nClient;            // 客户端的数量
	int m_clientfds[1024];    // 会话套接字集
};
extern FdQueue g_fdQueue;
#endif  // __SERVER_H__
