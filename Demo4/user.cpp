/* 实现用户类 */  
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include "user.h"
#include "errnos.h"
/* 构造器 */
CUser::CUser (int fd, int port, string const& ip, int userID)
{
	m_connfd = fd;
	m_port = port;
	m_ip = ip;
	m_userID = userID;
	cout << "new client " << m_ip << ':' << m_port << endl;

    m_lastSendHeartBeat = time (NULL); // 上次发送心跳时间
    m_lastRecvHeartBeat = time (NULL); // 用户上次心跳时间
    m_suspectUser = 0;    // 可疑用户标志位
}
/* 析构器 */
CUser::~CUser (void)
{
    close (m_connfd);
    cout << m_name << ' ' << m_ip << ':' << m_port << " close!" << endl;
}
/* 得到用户对应的会话套接字 */
int CUser::getUserFd (void)
{
	return m_connfd;
}
