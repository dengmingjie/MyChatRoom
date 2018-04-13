#ifndef  __SERVER_H__
#define  __SERVER_H__
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <map>
using namespace std;
#include "packet.h"
#include "except.h"
#define  PORT  8888
/* 声明服务器类 */
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
private:
    // 监听线程过程
    static void* run (void* arg);
    // 监听线程处理
    void* loopListen (void);
	short m_port;    // 服务器端口号
	string m_ip;     // 服务器IP地址
	int m_listenfd;  // 监听套接字
	pthread_t m_tid; // 线程标识
};

static int g_userID; // 用户ID
/* 得到用户ID */
int getUserID (void);

/* 声明接收缓存区类 */
class CRecvBuf
{
public:
	// 构造器
	CRecvBuf (void);

    int m_curSize;  // 当前接收长度
    int m_allSize;  // 数据报总长度
    bool m_isHeaderFull;  // 是否接收完报头
    char m_recvBuf[1500]; // 接收缓存区
	HEADER header;  // 报头
};

/* 声明发送缓存区类 */
class CSendBuf
{
public:
	// 构造器
	CSendBuf (void);

    int m_curSize;  // 当前发送长度
    int m_allSize;  // 数据报总长度
    bool m_isHeaderOver;  // 是否发送完报头
    char m_sendBuf[1500]; // 发送缓存区
	HEADER header;  // 报头
};

/* 声明用户类 */
class CUser
{
public:
    // 构造器
    CUser (int fd, int port, string const& ip);
    // 接收报体
    bool recvBody (int userID, CUser& user);
	// 发送报体
    bool sendBody (int userID, CUser& user);
    // 得到用户对应的会话套接字
    int getUserFd (void);

    int m_connfd;    // 会话套接字
    int m_port;      // 用户端口号
    string m_ip;     // 用户IP地址
    string m_name;   // 用户名
	string m_pwd;    // 用户密码
    CRecvBuf m_recvBuf;  // 接收缓存区
    CSendBuf m_sendBuf;  // 发送缓存区
};

/* 声明用户队列 */
class FdQueue 
{
public:
	// 构造器
	FdQueue (void);
	// 析构器
    ~FdQueue (void);
	// 压入用户
    void push (int userID, CUser const& user);
	// 弹出用户
    void pop (int userID, CUser const& user);
    // 获取客户端数量
	int getClients (void);
private:
	pthread_mutex_t m_mutex;  // 互斥锁
    int m_nClient;            // 客户端的数量
public:
    map<int, CUser> m_users;  // 用户ID和用户类映射
};
extern FdQueue g_fdQueue;

/* 声明会话线程类 */
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
	// 接收数据
	bool recvData (int userID, CUser& user) throw (RecvException);
	// 命令处理
    void commDeal (int userID, CUser& user) throw (ThreadException);
    // 整理接收缓存区
    void clearRecv (CUser& user) throw (ThreadException);
	// 发送数据
    bool sendData (int userID, CUser& user) throw (SendException);
    // 整理发送缓存区
    void clearSend (CUser& user) throw (ThreadException);
    pthread_t m_tid;  // 线程标识
};
#endif  // __SERVER_H__
