/* 声明client类 */
#ifndef  __CLIENT_H__
#define  __CLIENT_H__
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <pthread.h>
#include <iostream>
using namespace std;
#include "user.h"
#include "sendbuf.h"
#include "errnos.h"
#include "except.h"
#define  TCPPORT   8888  /* 服务器TCP端口号 */
#define  MAXUSERS   512  /* 最大用户数量 */
#define  MAXEVENTS  256  /* 最大事件数量 */
/* client类 */
class Client 
{
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
    // 启动数据I/O模块
    void start (void) throw (ThreadException);
private:
    // 线程过程
    void* run (void* arg);
    // 线程处理
    void loopEpoll (void) throw (ClientException);

    // 接收数据
    bool recvData (CRecvBuf* pRecvBuf) throw (RecvException);
    // 接收包体
    bool recvBody (CRecvBuf* pRecvBuf) throw (RecvException);
	// 发送数据
    bool sendData (CSendBuf* pSendBuf) throw (SendException);

    short m_port;    // 服务器端口号
    string m_ip;     // 服务器IP地址
    int m_epollfd;   // epoll描述符
    pthread_t m_tid; // 线程标识
};
#endif  // __CLIENT_H__
