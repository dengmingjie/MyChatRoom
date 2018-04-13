/* 声明服务器类 */
#ifndef  __SERVER_H__
#define  __SERVER_H__
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "user.h"
#include "except.h"
#define  PORT     8888  /* 服务器端口号 */
#define  LISTEN     50  /* 最大监听数量 */
#define  MAXUSERS  512  /* 最大用户数量 */
#define  MAXEVENTS 256  /* 最大事件数量 */
/* 服务器类 */
class CServer 
{
public:
	// 构造器
	CServer (short port, string const& ip = "")
		throw (ServerException);
	// 析构器
	~CServer (void);
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
/* 设置用户ID */
int setUserID (void);
#endif  // __SERVER_H__
