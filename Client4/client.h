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
#include "recvbuf.h"
#include "sendbuf.h"
#include "errnos.h"
#include "except.h"
#define  PORT      8888  /* 服务器端口号 */
#define  HEARTRATE   40  /* 心 跳 频 率 */
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
    // 启动指令线程
    void start (void) throw (ThreadException);
	// 循环监控
	void loopSelect (void) throw (ClientException);
private:
    // 服务器心跳检查
    void heartBeatCheck (void) throw (ClientException);
    // 接收数据
    bool recvData (void) throw (RecvException);
    // 接收包体
    bool recvBody (void) throw (RecvException);
	// 命令处理
    void commDeal (void) throw (ClientException);
    // 整理接收缓存区
    void clearRecv (void) throw (ClientException);
	// 发送数据
    bool sendData (void) throw (SendException);
    // 整理发送缓存区
    void clearSend (void) throw (ClientException);
    // 向服务器发送心跳
    void sendHeartBeat (void) throw (SendException);

	// 指令线程过程
	static void* run (void* arg);
	// 指令线程处理
	void* loopComm (void);
    // 登录指令处理
    void logInDeal (void);
    // 私聊指令处理
    void pChatDeal (void);
    // 群聊指令处理
    void gChatDeal (void);
    // 登出指令处理
    void logOutDeal (void);

	short m_port;  // 服务器端口号
	string m_ip;   // 服务器IP地址
	int m_sockfd;  // 套接字描述符
	int m_userID;  // 用户ID
    time_t m_lastSendHeartBeat; // 上次发送心跳时间
    time_t m_lastRecvHeartBeat; // 服务器上次心跳时间
    CRecvBuf m_recvBuf;  // 接收缓存区
    CSendBuf m_sendBuf;  // 发送缓存区
    pthread_t m_tid;  // 线程标识
};
#endif  // __CLIENT_H__
