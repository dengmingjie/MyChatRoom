/* 实现client类 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/ioctl.h>
#include <cstring>
#include <fcntl.h>     /* fcntl函数设置socket为非阻塞 */
#include "client.h"
#include "sendqueue.h"
#include "packet.h"
/* 构造器 */
Client::Client (short port, string const& ip/* = "127.0.0.1"*/)
	throw (ClientException): m_port(port), m_ip(ip) {}
/* 析构器 */
Client::~Client (void)
{
    close (m_epollfd);
}
/* 连接服务器 */
void Client::connectServer (void) throw (SocketException)
{
    cout << "连接服务器开始..." << endl;
    // 创建客户端UDP套接字
    m_UDPSocket = socket (AF_INET, SOCK_DGRAM, 0);
    if (-1 == m_UDPSocket)  // -1表示出错，抛异常
        throw SocketException (strerror (errno));

    // 初始化客户端UDP协议地址
    m_clientUDPAddr.sin_family = AF_INET;
    m_clientUDPAddr.sin_port = htons (rand() % 50000 + 6000);
    m_clientUDPAddr.sin_addr.s_addr = inet_addr ("127.0.0.1");

    // 绑定
    if (bind (m_UDPSocket, (struct sockaddr*)&m_clientUDPAddr, sizeof (m_clientUDPAddr)) < 0)
        throw SocketException (strerror (errno));

    // 创建客户端TCP套接字
    m_TCPSocket = socket (AF_INET, SOCK_STREAM, 0);
    if (-1 == m_TCPSocket)  // -1表示出错，抛异常
		throw SocketException (strerror (errno));

    // 设置m_TCPSocket为非阻塞
    int flags = fcntl (m_TCPSocket, F_GETFL, 0);
    if (fcntl (m_TCPSocket, F_SETFL, flags | O_NONBLOCK) < 0)
        throw ClientException (strerror (errno));

    // 初始化协议地址，用于m_TCPSocket和服务器端建立连接
	struct sockaddr_in servAddr;
    memset (&servAddr, 0, sizeof (servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (m_port);
	servAddr.sin_addr.s_addr = inet_addr (m_ip.c_str());
//	inet_aton (m_ip.c_str(), &servAddr.sin_addr);

CONNECT:
    // 向服务器发起连接请求，三次握手
    int ret = connect (m_TCPSocket, (struct sockaddr*)&servAddr, sizeof (servAddr));


    // m_TCPSocket若为非阻塞，connect函数调用后，无论是否建立连接，都会立即返回-1
    if (-1 == ret)
	{
		// 同时将errno设置为EINPROGRESS，表示此时三次握手仍在进行
        // 如果errno不是EINPROGRESS，则说明连接错误，再次请求连接
		if (errno != EINPROGRESS)
        {
            cout << "连接错误！再次请求连接中..." << endl;
			sleep (1);
            goto CONNECT;
        }
        else
        {
            fd_set rSet;  // 定义可读事件集合
            fd_set wSet;  // 定义可写事件集合
            struct timeval timeout;  // 设置等待时间
            /* NULL表示无限等待，0表示轮询 */

            FD_ZERO (&rSet);  // 清空可读事件集合
            FD_ZERO (&wSet);  // 清空可写事件集合
            FD_SET (m_TCPSocket, &rSet);
            FD_SET (m_TCPSocket, &wSet);

            // 5ms
            timeout.tv_sec = 0;     // 秒
            timeout.tv_usec = 5000;  // 毫秒

            int nReady = select (m_TCPSocket + 1, &rSet, &wSet, NULL, &timeout);
            if (nReady <= 0)
            {
                cout << "连接超时！再次请求连接中..." << endl;
                sleep (1);
                goto CONNECT;
            }
            else
            {
                int error = 0;
                int len = sizeof (error);
                getsockopt (m_TCPSocket, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&len);
                if (error)
                {
                    cout << "连接错误！再次请求连接中..." << endl;
                    sleep (1);
                    goto CONNECT;
                }
            }  // end else
        }  // end else
    }  // end if
    // 如果客户端和服务器在同一台主机上时，connect函数也会立即结束，并返回0？
    if (0 == ret)
	{
		cout << "connect completed..." << endl;
    }
	cout << "连接服务器完成！ " << endl;
}
/* 连接成功后，获取内核分配的本地协议地址 */
void Client::getLocalAddr (void) throw (ClientException)
{
	cout << "获取本地协议地址开始..." << endl;
	struct sockaddr_in localAddr;  // 本地协议地址
	char clientIP[20];  // 存储客户端IP地址
	socklen_t local_len = sizeof (localAddr);  
    memset (&localAddr, 0, sizeof (localAddr));

	// 获取内核分配的本地协议地址
    if (getsockname (m_TCPSocket, (struct sockaddr*)&localAddr, &local_len) == -1)
		throw SocketException (strerror (errno));

    inet_ntop (AF_INET, &localAddr.sin_addr, clientIP, sizeof (clientIP));

	// 在屏幕输出IP地址和端口号
	cout << "host " << clientIP << ':' << ntohs (localAddr.sin_port) << endl;
	cout << "获取本地协议地址完成！ " << endl;
}
/* 启动数据I/O模块 */
void Client::start (void) throw (ThreadException)
{
    cout << "启动数据I/O模块开始..." << endl;
    int error = pthread_create (&m_tid, NULL, run, this);
    if (error)
        throw ThreadException (strerror (errno));
    cout << "启动数据I/O模块完成！ " << endl;
}
/* 线程过程 */
void* Client::run (void* arg)
{
    pthread_detach (pthread_self());
    return static_cast<Client*> (arg)->loopEpoll();
}
/* 线程处理 */
void Client::loopEpoll (void) throw (ClientException)
{
    /* 声明epoll_event结构体的变量，ev用于注册事件，数组用于回传要处理的事件 */
    struct epoll_event ev, events[MAXEVENTS];
    m_epollfd = epoll_create (MAXUSERS);

    // 设置套接字描述符可读，默认水平触发模式
    ev.events = EPOLLIN;
    // 注册epoll事件
    epoll_ctl (m_epollfd, EPOLL_CTL_ADD, m_TCPSocket, &ev);

	// 循环监控
    while (1)
    {
        // 服务器心跳检查
        heartBeatCheck();

		// 判断是否有未发完的数据
        if (!g_sendQueue.empty())
		{
            // 设置套接字描述符可读可写，默认水平触发模式
            ev.events = EPOLLIN | EPOLLOUT;
            // 修改epoll事件
            epoll_ctl (m_epollfd, EPOLL_CTL_MOD, m_TCPSocket, &ev);
		}
        else
		{
            // 判断是否需要向服务器发送心跳
            sendHeartBeat();
        }

        // 等待epoll事件的发生
        int nReady = epoll_wait (m_epollfd, events, MAXEVENTS, 20);
        if (0 == nReady) // 超时
			continue;

        // 处理发生的所有事件
        for (int i = 0; i < nReady; ++i)
        {
            // 判断是否有可读事件
            if (events[i].events & EPOLLIN)
            {
                if (m_pRecvBuf == NULL)
                {
                    m_pRecvBuff = new CRecvBuf;
                }
                if (recvData (m_pRecvBuf))  // 接收数据
                {
                    g_cmdQueue.pushCmd (&m_pRecvBuf->header);  // 向上抛出命令
                    m_pRecvBuf = NULL;
                }
            }

            // 判断是否有可写事件
            if (events[i].events & EPOLLOUT)
            {
                if (sendData (g_sendQueue.frontSend()))  // 发送数据
                {
                    // 设置套接字描述符只读，默认水平触发模式
                    ev.events = EPOLLIN;
                    // 修改epoll事件
                    epoll_ctl (m_epollfd, EPOLL_CTL_MOD, m_TCPSocket, &ev);
                }
            }
        }  // end for
	}  // end while
}
/* 接收数据 */
bool Client::recvData (CRecvBuf* pRecvBuf) throw (RecvException)
{
    //cout << "接收数据开始..." << endl;
    if (pRecvBuf->m_isHeaderFull)  // 判断包头是否接收完
	{
        // 非阻塞接收包体，接收完返回true，否则返回false
        return recvBody (pRecvBuf);
    }
	else  // 若未接收完包头
	{
		int dataLen;
        if (ioctl (user.m_TCPSocket, FIONREAD, &dataLen) < 0)  // 判断底层buf是否收到包头
            return false;
        if (dataLen < sizeof (pRecvBuf->header))  // 未收到16个字节的包头
			return false;

        int ret = recv (user.m_TCPSocket, ＆pRecvBuf->header, sizeof (pRecvBuf->header), 0);  // 接收包头
        if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
        {
            cout << "recvData() ";
            errnos(); // 错误处理
            return false;
        }
        if (0 == ret) // 服务器关闭
 	    {
            cout << endl << "连接关闭！" << endl;
			throw RecvException (strerror (errno));
	    }

        cout << "接收包头完成！" << endl;
        pRecvBuf->m_curSize = sizeof (pRecvBuf->header);
        pRecvBuf->m_allSize = pRecvBuf->header.length;  // 确定包的长度
        pRecvBuf->m_isHeaderFull = true;
	}
	return false;
}
/* 接收包体 */
bool Client::recvBody (CRecvBuf* pRecvBuf) throw (RecvException)
{
    cout << "接收包体开始..." << endl;
    int ret = recv (user.m_TCPSocket, pRecvBuf->m_recvBuf + pRecvBuf->m_curSize - sizeof (pRecvBuf->header), pRecvBuf->m_allSize - pRecvBuf->m_curSize, MSG_DONTWAIT);  // 非阻塞接收
    if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
    {
        cout << "recvBody() ";
        errnos(); // 错误处理
        return false;
    }
    if (0 == ret) // 服务器关闭
	{
        cout << endl << "连接关闭！" << endl;
		throw RecvException (strerror (errno));
	}

    pRecvBuf->m_curSize += ret;  // 当前接收长度 = 原接收长度 + 实际接收长度
    if (pRecvBuf->m_allSize == pRecvBuf->m_curSize)  // 判断是否接收完
	{
        cout << "接收包体完成！" << endl;
		return true;
	}
    else
	{
        cout << "接收包体未完成..." << endl;
		return false;
	}
}
/* 发送数据 */
bool Client::sendData (CSendBuf* pSendBuf) throw (SendException)
{
    cout << "发送数据开始..." << endl;
    int ret = send (user.m_TCPSocket, pSendBuf->m_sendBuf + pSendBuf->m_curSize, pSendBuf->m_allSize - pSendBuf->m_curSize, MSG_DONTWAIT);  // 非阻塞发送
    if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
    {
        cout << "sendData() ";
        errnos(); // 错误处理
        return false;
    }
    if (0 == ret) // 服务器关闭
    {
        cout << endl << "连接关闭！" << endl;
        throw SendException (strerror (errno));
    }

    pSendBuf->m_curSize += ret;  // 当前发送长度 = 原发送长度 + 实际发送长度
    if (pSendBuf->m_allSize == pSendBuf->m_curSize)  // 判断是否发送完
    {
        cout << "发送数据完成！" << endl;
        g_sendQueue.popSend();
        delete pSendBuf;
        return true;
	}
    else
    {
        cout << "发送数据未完成..." << endl;
		return false;
	}
}
/* 向服务器发送心跳 */
void Client::sendHeartBeat (void) throw (SendException)
{
    time_t now = time (NULL);  // 获取系统当前时间
    // 判断是否到发送心跳时间
    if (now - user.m_lastSendHeartBeat < HEARTRATE)
    {
        // 未到发送心跳时间
        return;
    }
    else
    {
        if (user.m_userID < 0)  // 未登录，无需发心跳
            return;

        cout << "向服务器发送心跳！" << endl;
        HEADER header;
        header.version = VERSION;
        header.commID = heartBeat;
        header.length = sizeof (header);
        header.userID = m_userID;

        memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));  // 放到发送缓冲区
        m_sendBuf.m_allSize = header.length;  // 确定包的长度

        // 声明epoll_event结构体的变量用于修改事件
        struct epoll_event ev;
        // 设置套接字描述符可读可写，默认水平触发模式
        ev.events = EPOLLIN | EPOLLOUT;
        // 修改epoll事件
        epoll_ctl (m_epollfd, EPOLL_CTL_MOD, m_TCPSocket, &ev);

        // 更新上次发送心跳时间
        m_lastSendHeartBeat = now;
    }
}
