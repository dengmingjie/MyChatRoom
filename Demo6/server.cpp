/* 实现server类 */
#include <sys/types.h>  
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>    
#include <sys/ioctl.h>
#include <fcntl.h>     /* fcntl函数设置socket为非阻塞 */
#include "server.h"
#include "errnos.h"
#include "newqueue.h"
/* 构造器 */
CServer::CServer (short port, string const& ip/* = ""*/)
	throw (ServerException): m_port(port), m_ip(ip) {}
/* 析构器 */
CServer::~CServer (void) 
{
    close (m_listenfd); // 关闭监听套接字
    close (m_UDPfd);
	cout << "m_listenfd is closed..." << endl;
}
/* 初始化服务器 */
void CServer::initServer (void) throw (SocketException) 
{
	cout << "初始化服务器开始..." << endl;
    // 创建服务器UDP套接字
    m_UDPfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (-1 == m_UDPfd)  // -1表示出错，抛异常
        throw SocketException (strerror (errno));

    // 设置端口重用
    int n = 1;
    if (setsockopt (m_UDPfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n)) == -1)
        throw SocketException (strerror (errno));

    // 初始化服务器UDP协议地址
    struct sockaddr_in servUDPAddr;
    memset (&servUDPAddr, 0, sizeof (servUDPAddr));
    servUDPAddr.sin_family = AF_INET;
    servUDPAddr.sin_port = htons (UDPPORT);
    servUDPAddr.sin_addr.s_addr = htonl (INADDR_ANY);

    // 绑定
    if (bind (m_UDPfd, (struct sockaddr*)&servUDPAddr, sizeof(servUDPAddr)) == -1)
        throw SocketException (strerror (errno));

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
    if (listen (m_listenfd, LISTEN) == -1)
		throw SocketException (strerror (errno));

	cout << "初始化服务器完成！ " << endl;
}
/* 启动监听线程 */
void CServer::start (void) throw (ThreadException) 
{
	cout << "启动监听线程开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
	cout << "启动监听线程完成！ " << endl;
}
/* 监听线程过程 */
void* CServer::run (void* arg) 
{
	pthread_detach (pthread_self());
	return static_cast<CServer*> (arg)->loopListen();
}
/* 监听线程处理 */
void* CServer::loopListen (void) 
{
    /* 声明epoll_event结构体的变量，ev用于注册事件，数组用于回传要处理的事件 */
    struct epoll_event ev, events[MAXEVENTS];
    int epollfd = epoll_create (MAXUSERS);

	struct sockaddr_in clieAddr;
    socklen_t clieAddr_len = sizeof (clieAddr);

    // 设置关联监听套接字
    ev.data.fd = m_listenfd;
    // 设置监听套接字可读，默认水平触发模式
    ev.events = EPOLLIN;
    // 注册epoll事件
    epoll_ctl (epollfd, EPOLL_CTL_ADD, m_listenfd, &ev);

	cout << "listening..." << endl;
	// 循环监控
	while (1) 
	{
        // 等待epoll事件的发生
        int nReady = epoll_wait (epollfd, events, MAXEVENTS, 5);
        if (0 == nReady)  // 超时
			continue;

        // 处理发生的所有事件
        for (int i = 0; i < nReady; ++i)
        {
            // 判断是否有客户端请求连接
            if (events[i].data.fd == m_listenfd)
            {
                int connfd = accept (m_listenfd, (struct sockaddr*)&clieAddr, &clieAddr_len);
                if (-1 == connfd)
                    throw ServerException (strerror (errno));

                // 设置connfd为非阻塞
                int flags = fcntl (connfd, F_GETFL, 0);
                if (fcntl (connfd, F_SETFL, flags | O_NONBLOCK) < 0)
                    throw SocketException (strerror (errno));

                // 创建用户对象
                CUser* pUser = new CUser (connfd, setUserID(), clieAddr);
                // 向上抛新用户
                g_newQueue.pushNew (pUser);

            }  // end if
        }  // end for
	}  // end while

    return NULL;
}
/* 获取服务器UDP套接字 */
int CServer::getUDPfd (void)
{
    return m_UDPfd;
}

/* 设置用户ID */
int setUserID (void)
{
    return ++g_userID;
}
