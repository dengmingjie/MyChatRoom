/* 实现用户类 */  
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <iostream>
using namespace std;
#include "user.h"
CUser user;  // 定义用户
/* 构造器 */
CUser::CUser (void)
{
	m_userID = -1;
    m_packetNum = -1;
    m_lastSendHeartBeat = time (NULL);
    m_lastRecvHeartBeat = time (NULL);
    m_pRecvBuf = NULL;

    m_serverUDPAddr.sin_family = AF_INET;
    m_serverUDPAddr.sin_port = htons (UDPPORT);
    m_serverUDPAddr.sin_addr.s_addr = inet_addr ("127.0.0.1");
}
/* 析构器 */
CUser::~CUser (void)
{
    close (m_TCPSocket);
    close (m_UDPSocket);
}
/* 获取客户端TCP套接字 */
int CUser::getTCPSocket (void)
{
	return m_TCPSocket;
}
/* 获取客户端UDP套接字 */
int CUser::getUDPSocket (void)
{
	return m_UDPSocket;
}
/* 获取用户ID */
int CUser::getUserID (void)
{
	return m_userID;
}
