/* 声明用户类 */
#ifndef  __USER_H__
#define  __USER_H__
#include <iostream>
using namespace std;
#include "recvbuf.h"
#include "sendbuf.h"
#define  HEARTRATE   40  /* 心 跳 频 率 */
#define  SUSPECTUSER  5  /* 确定可疑用户 */
/* 用户类 */
class CUser
{
public:
    // 构造器
    CUser (int fd, int port, string const& ip, int userID);
	// 析构器
	~CUser (void);
    // 得到用户对应的会话套接字
    int getUserFd (void);

    int m_connfd;  // 会话套接字
	int m_userID;  // 用户ID
    int m_port;    // 用户端口号
    string m_ip;   // 用户IP地址
    char m_name[20]; // 用户名
    char m_pwd[20];  // 密码
    time_t m_lastSendHeartBeat; // 上次发送心跳时间
    time_t m_lastRecvHeartBeat; // 用户上次心跳时间
    CRecvBuf m_recvBuf;  // 接收缓存区
    CSendBuf m_sendBuf;  // 发送缓存区
    int m_suspectUser;   // 可疑用户标志位
};
#endif  // __USER_H__
