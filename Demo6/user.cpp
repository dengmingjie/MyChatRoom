/* 实现用户类 */  
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "user.h"
#include "errnos.h"
/* 构造器 */
CUser::CUser (int fd, int userID, struct sockaddr_in const& userTCPAddr)
{
	m_connfd = fd;
	m_userID = userID;
    m_userTCPAddr = userTCPAddr;
    cout << "new client " << inet_ntoa (m_userTCPAddr.sin_addr) << ':' << ntohs (m_userTCPAddr.sin_port) << endl;

    m_lastSendHeartBeat = time (NULL);  // 上次发送心跳时间
    m_lastRecvHeartBeat = time (NULL);  // 用户上次心跳时间

    m_pRecvBuf = NULL;
    m_sendBufEmpty = true;

    m_suspectUser = 0;  // 可疑用户标志位
    m_delete = 0;  // 待删除标志位，０正常，１待删除，２可删除
}
/* 析构器 */
CUser::~CUser (void)
{
    close (m_connfd);
    cout << m_name << ' ' << inet_ntoa (m_userTCPAddr.sin_addr) << ':' << ntohs (m_userTCPAddr.sin_port) << " close!" << endl;
}
/* 得到用户对应的会话套接字 */
int CUser::getUserFd (void)
{
	return m_connfd;
}
/* 得到用户对应的用户ID */
int CUser::getUserID (void)
{
    return m_userID;
}
/* 设置待删除标志位 */
void CUser::setDelete (int num)
{
    m_delete = num;
}
/* 查看待删除标志位 */
int CUser::getDelete (void)
{
    return m_delete;
}
