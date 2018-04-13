#include <sys/types.h>  
#include <sys/socket.h>    
#include <stdlib.h>  
#include <errno.h>  
#include <arpa/inet.h>  
#include <netinet/in.h>  
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>     /* fcntl函数设置socket为非阻塞 */
#include "server.h"
#include "errnos.h"
/* 实现server类 */
/* 构造器 */
Server::Server (short port, string const& ip/* = ""*/)
	throw (ServerException): m_port(port), m_ip(ip) {}
/* 析构器 */
Server::~Server (void) 
{
	close (m_listenfd);  // 关闭监听套接字
}
/* 初始化服务器 */
void Server::initServer (void) throw (SocketException) 
{
	cout << "初始化服务器开始..." << endl;
	// 创建服务器监听套接字
	m_listenfd = socket (AF_INET, SOCK_STREAM, 0);
	if (-1 == m_listenfd)  // -1表示出错，抛异常
		throw SocketException (strerror (errno));

	// 设置m_listenfd为非阻塞
	int flags = fcntl (m_listenfd, F_GETFL, 0);
	if (fcntl (m_listenfd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw SocketException (strerror (errno));
	
	// 设置端口重用
	int on = 1;
	if (setsockopt (m_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
		throw SocketException (strerror (errno));

	// 初始化服务器协议地址
	struct sockaddr_in servAddr;
    memset (&servAddr, 0, sizeof (servAddr));
	servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons (m_port);
    servAddr.sin_addr.s_addr = htonl (INADDR_ANY);

	// 绑定
	if (bind (m_listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		throw SocketException (strerror (errno));

	// 监听套接字
	if (listen (m_listenfd, 50) == -1)
		throw SocketException (strerror (errno));

	cout << "初始化服务器完成！ " << endl;
}
/* 启动监听线程 */
void Server::start (void) throw (ThreadException) 
{
	cout << "启动监听线程开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
	cout << "启动监听线程完成！ " << endl;
}
/* 监听线程过程 */
void* Server::run (void* arg) 
{
	pthread_detach (pthread_self());
	return static_cast<Server*> (arg)->loopListen();
}
/* 监听线程处理 */
void* Server::loopListen (void) 
{
	struct sockaddr_in clieAddr;
    socklen_t clieAddr_len = sizeof (clieAddr);
	fd_set rSet;  // 定义可读事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */

	cout << "listening..." << endl;
	// 循环监控
	while (1) 
	{
		FD_ZERO (&rSet);  // 清空可读事件集合
		FD_SET (m_listenfd, &rSet);  // 添加监听描述符
		timeout.tv_sec = 0;      // 秒
		timeout.tv_usec = 5000;  // 微秒
		int nReady = select (m_listenfd + 1, &rSet, NULL, NULL, &timeout);
        if (nReady < 0) // 失败返回-1，errno被设为以下的某个值
        {
            errnos();  // 错误处理
            continue;  // 继续
        }
        if (0 == nReady)  // 超时
			continue;

		// 判断是否有客户端请求连接
		if (FD_ISSET (m_listenfd, &rSet)) {
			int connfd = accept (m_listenfd, (struct sockaddr*)&clieAddr, &clieAddr_len);  // 不会阻塞
			if (-1 == connfd) 
				throw ServerException (strerror (errno));

			// 设置connfd为非阻塞
			int flags = fcntl (connfd, F_GETFL, 0);
			if (fcntl (connfd, F_SETFL, flags | O_NONBLOCK) < 0)
				throw SocketException (strerror (errno));
            
			// 压入用户队列
			CUser user(connfd, ntohs(clieAddr.sin_port), inet_ntoa(clieAddr.sin_addr));  // 创建用户对象
            g_fdQueue.push (getUserID(), user);
		}
	}  // end while
}

/* 实现会话线程类 */
/* 启动会话线程 */
void TalkThread::start (void) throw (ThreadException) 
{
	cout << "启动会话线程开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
	cout << "启动会话线程完成！ " << endl;
}
/* 会话线程过程 */
void* TalkThread::run (void* arg) 
{
	pthread_detach (pthread_self());
	return static_cast<TalkThread*> (arg)->loopTalk();
}
/* 会话线程处理 */
void* TalkThread::loopTalk (void) 
{
	fd_set rSet;  // 定义可读事件集合
	fd_set wSet;  // 定义可写事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */

	while (1) 
	{
		FD_ZERO (&rSet);  // 清空可读事件结合
		FD_ZERO (&wSet);  // 清空可写事件结合
        for (map<int, CUser>::iterator it = g_fdQueue.m_users.begin(); it != g_fdQueue.m_users.end(); ++it)
		{
            FD_SET (it->second.m_connfd, &rSet);  // 添加到可读事件集合

            if (it->second.m_sendBuf.m_allSize != it->second.m_sendBuf.m_curSize)  // 判断是否有未发完的数据
                FD_SET (it->second.m_connfd, &wSet);  // 添加到可写事件集合

            if (it->second.m_connfd > m_maxfd)
                m_maxfd = it->second.m_connfd;  // 确定集合最大有效下标
        }
		timeout.tv_sec = 0;      // 秒
		timeout.tv_usec = 5000;  // 微秒
		int nReady = select (m_maxfd + 1, &rSet, &wSet, NULL, &timeout);
        if (nReady < 0) // 失败返回-1，errno被设为以下的某个值
        {
            errnos();  // 错误处理
            continue;  // 继续
        }
		if (0 == nReady)  // 超时
			continue;
			
        for (map<int, CUser>::iterator it = g_fdQueue.m_users.begin(); it != g_fdQueue.m_users.end(); ++it)
		{
			// 判断是否有可读事件
            if (FD_ISSET (it->second.m_connfd, &rSet))
			{
				if (recvData (it->first, it->second))  // 接收数据
					commDeal (it->first, it->second);  // 命令处理
            }

			// 判断是否有可写事件
            if (FD_ISSET (it->second.m_connfd, &wSet))
			{
                if (sendData (it->first, it->second))  // 发送数据
                    clearSend (it->second);  // 清理发送缓存区
			}
		}  // end for
	}  // end while
}
/* 接收数据 */
bool TalkThread::recvData (int userID, CUser& user) throw (RecvException)
{
	if (user.m_recvBuf.m_isHeaderFull)  // 判断报头是否接收完
	{
        // 非阻塞接收报体，接收完返回true，否则返回false
        return user.recvBody (userID, user);
    }
	else  // 若未接收完报头
	{
		int dataLen;
        if (ioctl (user.m_connfd, FIONREAD, &dataLen) < 0)  // 判断底层buf是否收到报头
            return false;
		if (dataLen < sizeof (user.m_recvBuf.header))  // 未收到20个字节的报头
			return false;

		int ret = recv (user.m_connfd, &user.m_recvBuf.header, sizeof (user.m_recvBuf.header), 0);  // 接收报头
        if (ret < 0) // 失败返回-1，errno被设为以下的某个值
        {
            errnos();  // 错误处理
            return false;
        }
        if (0 == ret) // 客户端关闭
        {
            g_fdQueue.pop (userID, user);  // 弹出用户
			return false;
        }
		// 接收完报头
		user.m_recvBuf.m_curSize = sizeof (user.m_recvBuf.header);
		user.m_recvBuf.m_allSize = user.m_recvBuf.header.length;
		user.m_recvBuf.m_isHeaderFull = true;
	}
	return false;
}
/* 命令处理 */
void TalkThread::commDeal (int userID, CUser& user) throw (ThreadException)
{
	if (user.m_recvBuf.header.commID == logIn)  // 登录
    {
        memcpy (&user.m_name, user.m_recvBuf.m_recvBuf, 20);  // 得到用户名
		memcpy (&user.m_pwd, user.m_recvBuf.m_recvBuf+20, 20);  // 得到密码
        user.m_sendBuf.header.version = user.m_recvBuf.header.version;
        user.m_sendBuf.header.commID = LogIn_SUCCESS;
        user.m_sendBuf.header.length = sizeof (user.m_sendBuf.header);
        user.m_sendBuf.header.userID = userID;
        user.m_sendBuf.header.to_userID = userID;
        clearRecv(user);  // 整理接收缓存区
    }

	if (user.m_recvBuf.header.commID == pChat)  // 私聊
	{
        cout << "pChat: " << user.m_recvBuf.m_recvBuf << endl;
        map<int, CUser>::iterator it = g_fdQueue.m_users.find(user.m_recvBuf.header.to_userID);
        memcpy (it->second.m_sendBuf.m_sendBuf, user.m_recvBuf.m_recvBuf+40, 1500-40);
        it->second.m_sendBuf.header.version = user.m_recvBuf.header.version;
        it->second.m_sendBuf.header.commID = user.m_recvBuf.header.commID;
        it->second.m_sendBuf.header.length = sizeof (user.m_sendBuf.header) + sizeof (user.m_sendBuf.m_sendBuf);
        it->second.m_sendBuf.header.userID = userID;
        it->second.m_sendBuf.header.to_userID = user.m_recvBuf.header.to_userID;
        clearRecv(user);  // 整理接收缓存区
	}

	if (user.m_recvBuf.header.commID == gChat)  // 群聊
	{
        cout << "gChat: " << user.m_recvBuf.m_recvBuf << endl;
        for (map<int, CUser>::iterator it = g_fdQueue.m_users.begin(); it != g_fdQueue.m_users.end(); ++it)
        {
            memcpy (it->second.m_sendBuf.m_sendBuf, user.m_recvBuf.m_recvBuf+40, 1500-40);
            it->second.m_sendBuf.header.version = user.m_recvBuf.header.version;
            it->second.m_sendBuf.header.commID = user.m_recvBuf.header.commID;
            it->second.m_sendBuf.header.length = sizeof (user.m_sendBuf.header) + sizeof (user.m_sendBuf.m_sendBuf);
            it->second.m_sendBuf.header.userID = userID;
            it->second.m_sendBuf.header.to_userID = user.m_recvBuf.header.to_userID;
        }
        clearRecv(user);  // 整理接收缓存区
	}

	if (user.m_recvBuf.header.commID == logOut) // 登出
	{
		g_fdQueue.pop (userID, user);  // 弹出用户
	}
}
/* 整理接收缓存区 */
void TalkThread::clearRecv (CUser &user) throw (ThreadException)
{
    user.m_recvBuf.m_curSize = 0;
    user.m_recvBuf.m_allSize = 0;
    user.m_recvBuf.m_isHeaderFull = false;
    memset (user.m_recvBuf.m_recvBuf, 0, sizeof (user.m_recvBuf.m_recvBuf));
    memset (&user.m_recvBuf.header, 0, sizeof (user.m_recvBuf.header));
}
/* 发送数据 */
bool TalkThread::sendData (int userID, CUser& user) throw (SendException)
{
	if (!user.m_sendBuf.m_isHeaderOver)  // 若未发送完报头
    {
/*
		int dataLen;
        if (ioctl (user.m_connfd, FIONWRITE, &dataLen) < 0)
            return false;
		if (dataLen < sizeof (user.m_sendBuf.header))
            return false;
*/
		int ret = send (user.m_connfd, &user.m_sendBuf.header, sizeof (user.m_sendBuf.header), 0);  // 发送报头
        if (ret < 0) // 失败返回-1，errno被设为以下的某个值
        {
            errnos();  // 错误处理
            return false;
        }
        if (0 == ret) // 客户端关闭
        {
            g_fdQueue.pop (userID, user);  // 弹出用户
			return false;
        }
		// 发送完报头
		user.m_sendBuf.m_isHeaderOver = true;
	}

	if (user.m_sendBuf.m_isHeaderOver)  // 判断报头是否发送完
	{
        if (user.m_sendBuf.header.to_userID != userID)  // 废包
        {
            return true;
        }
		// 非阻塞发送报体，发送完返回true，否则返回false
        return user.sendBody (userID, user);
    }
}
// 整理发送缓存区
void TalkThread::clearSend (CUser& user) throw (ThreadException)
{
	user.m_sendBuf.m_curSize = 0;
    user.m_sendBuf.m_allSize = 0;
    user.m_sendBuf.m_isHeaderOver = false;
    memset (user.m_sendBuf.m_sendBuf, 0, sizeof (user.m_sendBuf.m_sendBuf));
    memset (&user.m_sendBuf.header, 0, sizeof (user.m_sendBuf.header));
}

/* 实现用户队列 */
FdQueue g_fdQueue;  // 定义全局队列
/* 构造器 */
FdQueue::FdQueue (void)
{
    pthread_mutex_init (&m_mutex, NULL);
}
/* 析构器 */
FdQueue::~FdQueue (void) 
{
    pthread_mutex_destroy (&m_mutex);
}
/* 压入用户 */
void FdQueue::push (int userID, CUser const& user)
{
	cout << "压入用户开始..." << endl;
    pthread_mutex_lock (&m_mutex);
    g_fdQueue.m_users.insert (pair<int, CUser> (userID, user));  // 压入
    m_nClient++;  // 客户端数量+1
	cout << "clients: " << m_nClient << endl;
	pthread_mutex_unlock (&m_mutex);
    cout << "压入用户完成！" << endl << endl;
}
/* 弹出用户 */
void FdQueue::pop (int userID, CUser const& user)
{
	cout << "弹出用户开始..." << endl;
    pthread_mutex_lock (&m_mutex);
    cout << "client " << user.m_ip << ':' << user.m_port << " closed!" << endl;
    close (user.m_connfd);  // 关闭对应套接字
    g_fdQueue.m_users.erase (userID);  // 弹出
    m_nClient--;    // 客户端数量-1
	cout << "clients: " << m_nClient << endl;
	pthread_mutex_unlock (&m_mutex);
    cout << "弹出用户完成！" << endl << endl;
}
/* 获取客户端数量 */
int FdQueue::getClients (void) 
{
	return m_nClient;
}

/* 实现用户类 */
/* 构造器 */
CUser::CUser (int fd, int port, string const& ip)
{
	m_connfd = fd;
	m_port = port;
	m_ip = ip;
	cout << "new client " << m_ip << ':' << m_port << endl;
}
/* 接收报体 */
bool CUser::recvBody (int userID, CUser & user)
{
    int ret = recv (user.m_connfd, user.m_recvBuf.m_recvBuf + user.m_recvBuf.m_curSize, user.m_recvBuf.m_allSize - user.m_recvBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞接收
    if (ret < 0) // 失败返回-1，errno被设为以下的某个值
    {
        errnos();  // 错误处理
        return false;
    }
    if (0 == ret) // 客户端关闭
    {
        g_fdQueue.pop (userID, user);  // 弹出用户
        return false;
    }

    user.m_recvBuf.m_curSize += ret;  // 当前接收长度 = 原接收长度 + 实际接收长度
    if (user.m_recvBuf.m_allSize == user.m_recvBuf.m_curSize)  // 判断是否接收完
	{
        user.m_recvBuf.m_isHeaderFull = false;
		return true;
	}
    else  // 未接收完
	{
		return false;
	}
}
/* 发送报体 */
bool CUser::sendBody (int userID, CUser & user)
{
    int ret = send (user.m_connfd, user.m_sendBuf.m_sendBuf + user.m_sendBuf.m_curSize, user.m_sendBuf.m_allSize - user.m_sendBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞发送
    if (ret < 0) // 失败返回-1，errno被设为以下的某个值
    {
        errnos();  // 错误处理
        return false;
    }
    if (0 == ret)  // 客户端关闭
    {
        g_fdQueue.pop (userID, user);  // 弹出用户
        return false;
    }

    user.m_sendBuf.m_curSize += ret;  // 当前发送长度 = 原发送长度 + 实际发送长度
    if (user.m_sendBuf.m_allSize == user.m_sendBuf.m_curSize)  // 判断是否发送完
    {
        // 发送完
        return true;
	}
    else
    {
        // 未发完
		return false;
	}
}

/* 得到用户ID */
int getUserID (void)
{
	return ++g_userID;
}

/* 实现接收缓存区类 */
/* 构造器 */
CRecvBuf::CRecvBuf (void)
{
    m_curSize = 0;  // 当前接收长度
    m_allSize = 0;  // 数据报总长度
    m_isHeaderFull = false;  // 是否接收完报头
    memset (m_recvBuf, 0, sizeof (m_recvBuf)); // 接收缓存区
    memset (&header, 0, sizeof (header));  // 报头
}

/* 实现发送缓存区类 */
/* 构造器 */
CSendBuf::CSendBuf (void)
{
    m_curSize = 0;  // 当前发送长度
    m_allSize = 0;  // 数据报总长度
    m_isHeaderOver = false;  // 是否发送完报头
    memset (m_sendBuf, 0, sizeof (m_sendBuf)); // 发送缓存区
    memset (&header, 0, sizeof (header));  // 报头
}
