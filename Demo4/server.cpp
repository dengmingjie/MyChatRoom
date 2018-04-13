/* 实现server类 */
#include <sys/types.h>  
#include <sys/socket.h>     
#include <arpa/inet.h>  
#include <netinet/in.h>  
#include <sys/ioctl.h>
#include <fcntl.h>     /* fcntl函数设置socket为非阻塞 */
#include "server.h"
#include "errnos.h"
#include "queue.h"
/* 构造器 */
CServer::CServer (short port, string const& ip/* = ""*/)
	throw (ServerException): m_port(port), m_ip(ip) {}
/* 析构器 */
CServer::~CServer (void) 
{
	close (m_listenfd);  // 关闭监听套接字
	cout << "m_listenfd is closed..." << endl;
}
/* 初始化服务器 */
void CServer::initServer (void) throw (SocketException) 
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

            // 创建用户对象
            CUser* pUser = new CUser (connfd, ntohs(clieAddr.sin_port), inet_ntoa(clieAddr.sin_addr), setUserID());
            // 压入用户队列
            g_queue.push (pUser);
		}
	}  // end while
}

/* 设置用户ID */
int setUserID (void)
{
    return ++g_userID;
}
