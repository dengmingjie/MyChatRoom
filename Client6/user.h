/* 声明用户类 */
#ifndef  __USER_H__
#define  __USER_H__
#include "stdio.h"
#include <netinet/in.h>
#include "recvbuf.h"
#define  UDPPORT   6666  /* 服务器UDP端口号 */
#define  VERSION      5  /* 客户端版本号 */
#define  HEARTRATE   40  /* 心 跳 频 率 */
/* 用户类 */
class CUser
{
public:
    // 构造器
    CUser (void);
	// 析构器
	~CUser (void);
    // 获取客户端TCP套接字
    int getTCPSocket (void);
    // 获取客户端UDP套接字
    int getUDPSocket (void);
    // 获取用户ID
    int getUserID (void);

    struct sockaddr_in m_clientUDPAddr; // 客户端UDP协议地址
    struct sockaddr_in m_serverUDPAddr; // 服务器UDP协议地址
    int m_TCPSocket;  // 客户端TCP套接字
    int m_UDPSocket;  // 客户端UDP套接字
    int m_userID;     // 用户ID
    time_t m_lastSendHeartBeat;  // 上次发送心跳时间
    time_t m_lastRecvHeartBeat;  // 服务器上次心跳时间
    CRecvBuf* m_pRecvBuf; // 接收缓存区

    char m_filePath[100]; // 文件路径
    long m_fileSend;      // 文件当前发送大小
    int m_fileRecvID;     // 文件接收方ID
    bool m_fileSendYes;   // 文件接收方接受与否
    FILE* m_fp;
    int m_packetNum;      // 包号

    char m_fileName[20];  // 文件名
    long m_fileSize;      // 文件大小
    int m_fileSendID;     // 文件发送方ID
};
extern CUser user;
#endif  // __USER_H__
