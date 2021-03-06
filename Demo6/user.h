/* 声明用户类 */
#ifndef  __USER_H__
#define  __USER_H__
#include <netinet/in.h>
#include <iostream>
using namespace std;
#include "recvbuf.h"
#include "sendbuf.h"
#define  VERSION      5  /* 服务器版本号 */
#define  HEARTRATE   40  /* 心 跳 频 率 */
#define  SUSPECTUSER  5  /* 确定可疑用户 */
/* 用户类 */
class CUser
{
public:
    // 构造器
    CUser (int fd, int userID, struct sockaddr_in const& userTCPAddr);
	// 析构器
	~CUser (void);
    // 得到用户对应的会话套接字
    int getUserFd (void);
    // 得到用户对应的用户ID
    int getUserID (void);
    // 设置待删除标志位
    void setDelete (int num);
    // 查看待删除标志位
    int getDelete (void);

    int m_connfd;    // 会话套接字
    int m_userID;    // 用户ID
    struct sockaddr_in m_userTCPAddr;  // 用户TCP协议地址
    struct sockaddr_in m_userUDPAddr;  // 用户UDP协议地址
    char m_name[20]; // 用户名
    char m_pwd[20];  // 密码
    time_t m_lastSendHeartBeat; // 上次发送心跳时间
    time_t m_lastRecvHeartBeat; // 用户上次心跳时间
    CRecvBuf* m_pRecvBuf; // 接收缓存区
    CSendBuf m_sendBuf;   // 发送缓存区
    bool m_sendBufEmpty;

    int m_suspectUser;  // 可疑用户标志位
    int m_delete;       // 待删除标志位：０正常 １待删除 ２可删除
};
#endif  // __USER_H__
