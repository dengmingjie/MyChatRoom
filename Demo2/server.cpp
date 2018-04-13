// 实现server类
#include <sys/types.h>  
#include <sys/socket.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <arpa/inet.h>  
#include <netinet/in.h>  
#include <pthread.h>
#include <string.h> 
#include "server.h"
#include "packet.h"

// 构造器
Server::Server (short port, string const& ip/* = ""*/)
	throw (ServerException): m_port(port), m_ip(ip) {}
// 析构器
Server::~Server (void) {}
// 初始化服务器
void Server::initServer(void) throw (SocketException) 
{
	cout << "初始化服务器开始..." << endl;
	// 创建服务器监听套接字
	m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == m_listenfd)  // -1表示出错，抛异常
		throw SocketException (strerror (errno));
	
	// 设置端口重用
	int on = 1;
	if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
		throw SocketException (strerror (errno));

	// 初始化服务器协议地址
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(m_port);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 绑定
	if (bind(m_listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		throw SocketException (strerror (errno));

	// 监听套接字
	if (listen(m_listenfd, 50) == -1)
		throw SocketException (strerror (errno));

	cout << "初始化服务器完成！ " << endl;
}
// 启动监听线程
void Server::start (void) throw (ThreadException) 
{
	cout << "启动监听线程开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
	cout << "启动监听线程完成！ " << endl;
}
// 监听线程过程
void* Server::run (void* arg) 
{
	pthread_detach (pthread_self ());
	return static_cast<Server*> (arg)->loopListen ();
}
// 监听线程处理
void* Server::loopListen (void) 
{
	struct sockaddr_in clieAddr;
	socklen_t clieAddr_len = sizeof(clieAddr);
	fd_set rSet;  // 定义可读事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */
		
	cout << "listening..." << endl;
	// 循环监控
	while (1) 
	{
		FD_ZERO(&rSet);  // 清空
		FD_SET(m_listenfd, &rSet);  // 添加监听描述符
		timeout.tv_sec = 0;      // 秒
		timeout.tv_usec = 5000;  // 微秒
		int nReady = select(m_listenfd + 1, &rSet, NULL, NULL, &timeout);
		if (nReady < 0) 
		{
			if (errno == EINTR)
				continue;
		}
		if ( 0 == nReady)  // 超时
			continue;

		// 判断是否有客户端请求连接
		if (FD_ISSET(m_listenfd, &rSet)) {
			int connfd = accept(m_listenfd, (struct sockaddr*)&clieAddr, &clieAddr_len);  // 不会阻塞
			if (-1 == connfd) 
				throw ServerException (strerror (errno));

			// 添加到会话套接字队列
			g_fdQueue.push(connfd);

			// 输出客户端IP地址和端口号
			cout << "new client: " << inet_ntoa(clieAddr.sin_addr) << ':' << ntohs(clieAddr.sin_port) << endl;
			// 获取客户端数量
			cout << "clients: " << g_fdQueue.getClients() << endl;
		}
	}  // end while
}

// 启动会话线程
void TalkThread::start (void) throw (ThreadException) 
{
	cout << "启动会话线程开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
	cout << "启动会话线程完成！ " << endl;
}
// 会话线程过程
void* TalkThread::run (void* arg) 
{
	pthread_detach (pthread_self ());
	return static_cast<TalkThread*> (arg)->loopTalk ();
}
// 会话线程处理
void* TalkThread::loopTalk (void) 
{
	fd_set rSet;  // 定义可读事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */
	while (1) 
	{
		FD_ZERO(&rSet);  // 清空结合
		for (int i = 0; i <= g_fdQueue.getClients(); i++) 
		{
			if (g_fdQueue.getClientfd(i) > 0) 
			{
				FD_SET(g_fdQueue.getClientfd(i), &rSet);  // 依次添加到集合
				if (g_fdQueue.getClientfd(i) > m_maxfd)
					m_maxfd = g_fdQueue.getClientfd(i);
			}
		}
		timeout.tv_sec = 0;      // 秒
		timeout.tv_usec = 5000;  // 微秒
		int nReady = select(m_maxfd + 1, &rSet, NULL, NULL, &timeout);
		if (0 == nReady)  // 超时
			continue;
			
		// 判断是否有客户端发来消息
		for (int i = 0; i <= g_fdQueue.getClients(); i++) 
		{
			if (-1 == g_fdQueue.getClientfd(i)) 
				continue;
			// 若有消息
			if (FD_ISSET(g_fdQueue.getClientfd(i), &rSet)) 
			{
				PACKET packet;
				memset(&packet, 0, sizeof(packet));
				int ret = recv(g_fdQueue.getClientfd(i), &packet, sizeof(packet), 0);
				if (ret < 0)  // 失败返回-1，errno被设为以下的某个值 
				{
					switch (errno)
					{
					case EAGAIN:
						cout << "EAGAIN: 套接字已标记为非阻塞，而接收操作被阻塞或者接收超时!" << endl;
						// 正常，不做处理
 						break;
					case EINTR:
						cout << "EINTR: 操作被信号中断!" << endl;
						// 正常，不做处理
 						break;
					case EBADF:
						cout << "EBADF: socket不是有效的套接字！" << endl;
						// 出错，提示错误信息
 						break;
					case EINVAL:
						cout << "EINVAL: 传给系统的参数无效!" << endl;
						// 出错，提示错误信息
 						break;
					case EFAULT:
						cout << "EFAULT: 内存空间访问出错!" << endl;
						// 出错，提示错误信息
 						break;
					case ENOMEM:
						cout << "ENOMEM: 核心内存不足！" << endl;
						// 出错，做相关处理
 						break;
						// SOCKET_ERROR: 网络错误！
						// ECONNREFUSE: 远程主机阻绝网络连接!
						// ENOTCONN: 与面向连接关联的套接字尚未被连接上！
						// ENOTSOCK: socket不是套接字！
						// errno == EWOULDBLOCK 正常，不做处理
					}
					// 继续
					continue;
				}

				if (0 == ret)  // 客户端关闭
				{  
					cout << endl << "client close!" << endl;
					g_fdQueue.pop(g_fdQueue.getClientfd(i));
					if (g_fdQueue.getClientfd(i) > m_maxfd) 
						m_maxfd--;
					cout << "clients: " << g_fdQueue.getClients() << endl;
					continue;
				}

				cout << "message: " << endl;  // 测试收到消息
				cout << packet.msg_len << ": " << packet.msg_buf << endl;

				// 向所有用户转发消息
				for (int i = 0; i <= g_fdQueue.getClients(); i++) 
				{
					if (-1 == g_fdQueue.getClientfd(i)) 
						continue;
					int ret = send(g_fdQueue.getClientfd(i), &packet, sizeof(packet), 0);  // 发送
					if (ret < 0)  // 发送失败，错误原因存于全局变量errno中
					{
						switch (errno)
						{
						case EAGAIN:
							cout << "EAGAIN: 套接字已标记为非阻塞，而接收操作被阻塞或者接收超时!" << endl;
							// 正常，不做处理
 							break;
						case EINTR:
							cout << "EINTR: 操作被信号中断!" << endl;
							// 正常，不做处理
 							break;
						case EBADF:
							cout << "EBADF: socket不是有效的套接字！" << endl;
							// 出错，提示错误信息
 							break;
						case EINVAL:
							cout << "EINVAL: 传给系统的参数无效!" << endl;
							// 出错，提示错误信息
 							break;
						case EFAULT:
							cout << "EFAULT: 内存空间访问出错!" << endl;
							// 出错，提示错误信息
 							break;
						case ENOMEM:
							cout << "ENOMEM: 核心内存不足！" << endl;
							// 出错，做相关处理
 							break;
							// SOCKET_ERROR: 网络错误！
							// ENOTSOCK: socket不是套接字！
							// ENOBUFS: 系统的缓冲内存不足！
						}
						// 继续
						continue;
					}

					if (0 == ret) {  // 客户端关闭
						cout << endl << "client close!" << endl;
						g_fdQueue.pop(g_fdQueue.getClientfd(i));
						if (g_fdQueue.getClientfd(i) > m_maxfd)
							m_maxfd--;
						cout << "clients: " << g_fdQueue.getClients() << endl;
					}
				}  // end for
			}  // end if
		}  // end for
	}  // end while
}

FdQueue g_fdQueue;
// 构造器
FdQueue::FdQueue (void) 
{
	pthread_mutex_init (&m_mutex, NULL);
	for (int i = 0; i < 1024; i++) 
		m_clientfds[i] = -1;
}
// 析构器
FdQueue::~FdQueue (void) 
{
	pthread_mutex_destroy (&m_mutex);
	// 关闭所有套接字
	for (int i = 0; i <= 1024; i++) 
	{
	    if (-1 != m_clientfds[i]) 
	        close(m_clientfds[i]);
	}   
}
// 压入会话套接字
void FdQueue::push (int connfd) 
{
	cout << "压入会话套接字开始..." << endl;
	pthread_mutex_lock (&m_mutex);
	for (int i = 0; i < 1024; i++) 
	{
		if (m_clientfds[i] < 0) 
		{
			m_clientfds[i] = connfd;  // 压入
			m_nClient++;  // 客户端数量+1
			break;
		}
	}
	pthread_mutex_unlock (&m_mutex);
	cout << "压入会话套接字完成！" << endl;
}
// 弹出会话套接字
void FdQueue::pop (int connfd) 
{
	cout << "弹出会话套接字开始..." << endl;
	pthread_mutex_lock (&m_mutex);
	for (int i = 0; i < 1024; i++) 
	{
		if (m_clientfds[i] == connfd) 
		{
            close(m_clientfds[i]); // 关闭其会话套接字
            m_clientfds[i] = -1;   // 置为-1
            m_nClient--;           // 客户端数量-1
			break;
		}
	}
	pthread_mutex_unlock (&m_mutex);
	cout << "弹出会话套接字完成！" << endl;
}
// 获取客户端数量
int FdQueue::getClients (void) 
{
	return m_nClient;
}
// 获取会话套接字集成员
int FdQueue::getClientfd (int num) 
{
	return m_clientfds[num];
}

