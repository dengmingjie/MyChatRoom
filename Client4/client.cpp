/* 实现client类 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <sys/ioctl.h>
#include <cstring>
#include <fcntl.h>     /* fcntl函数设置socket为非阻塞 */
#include "client.h"
#include "packet.h"
/* 构造器 */
Client::Client (short port, string const& ip/* = "127.0.0.1"*/)
	throw (ClientException): m_port(port), m_ip(ip) 
{
	m_lastSendHeartBeat = time (NULL);  // 上次发送心跳时间
	m_lastRecvHeartBeat = time (NULL);  // 服务器上次心跳时间
    m_userID = -1;
}
/* 析构器 */
Client::~Client (void)
{
	close (m_sockfd);  // 关闭客户端套接字
	cout << "m_sockfd is closed..." << endl;  // 测试
}
/* 连接服务器 */
void Client::connectServer (void) throw (SocketException)
{
	cout << "连接服务器开始..." << endl;
	// 创建客户端套接字
	m_sockfd = socket (AF_INET, SOCK_STREAM, 0);
	if (-1 == m_sockfd)  // -1表示出错，抛异常 
		throw SocketException (strerror (errno));

    // 设置m_sockfd为非阻塞
    int flags = fcntl (m_sockfd, F_GETFL, 0);
    if (fcntl (m_sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw ClientException (strerror (errno));

	// 初始化协议地址，用于m_sockfd和服务器端建立连接
	struct sockaddr_in servAddr;
	memset (&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons (m_port);
	servAddr.sin_addr.s_addr = inet_addr (m_ip.c_str());
//	inet_aton (m_ip.c_str(), &servAddr.sin_addr);

	// 向服务器发起连接请求，三次握手
	int ret = connect (m_sockfd, (struct sockaddr*)&servAddr, sizeof (servAddr));

    // m_sockfd若为非阻塞，connect函数调用后，无论是否建立连接，都会立即返回-1
    if (-1 == ret)
	{
		// 同时将errno设置为EINPROGRESS，表示此时三次握手仍在进行
        // 如果errno不是EINPROGRESS，则说明连接错误，程序结束
		if (errno != EINPROGRESS)
			throw SocketException (strerror (errno));
	}
	// 如果客户端和服务器在同一台主机上时，connect也会立即结束，并返回0
	if ( 0 == ret)
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
    if (getsockname (m_sockfd, (struct sockaddr*)&localAddr, &local_len) == -1)
		throw SocketException (strerror (errno));

    inet_ntop (AF_INET, &localAddr.sin_addr, clientIP, sizeof (clientIP));

	// 在屏幕输出IP地址和端口号
	cout << "host " << clientIP << ':' << ntohs (localAddr.sin_port) << endl;
	cout << "获取本地协议地址完成！ " << endl;
}
/* 循环监控 */
void Client::loopSelect (void) throw (ClientException)
{
	fd_set rSet;  // 定义可读事件集合
	fd_set wSet;  // 定义可写事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */

	// 循环监控
    while (1)
    {
        // 服务器心跳检查
        heartBeatCheck();

		FD_ZERO (&rSet);  // 清空可读事件集合
        FD_ZERO (&wSet);  // 清空可写事件集合
		FD_SET (m_sockfd, &rSet);

		// 判断是否有未发完的数据
        if (m_sendBuf.m_allSize != m_sendBuf.m_curSize)
		{
            FD_SET (m_sockfd, &wSet);  // 添加到可写事件集合
		}
		else
		{
			// 若没有，判断是否需要向服务器发送心跳
    		sendHeartBeat();
		}
        // 5ms
        timeout.tv_sec = 0;      // 秒
        timeout.tv_usec = 5000;  // 毫秒

        int nReady = select (m_sockfd + 1, &rSet, &wSet, NULL, &timeout);
        if (nReady < 0)  // 失败返回-1，errno被设为以下的某个值
        {
            errnos();  // 错误处理
            continue;  // 继续
        }
        if (0 == nReady) // 超时
			continue;

		// 判断是否有可读事件
        if (FD_ISSET (m_sockfd, &rSet))
        {
			if (recvData())  // 接收数据
			{
				commDeal();  // 命令处理
			}
		}

		// 判断是否有可写事件
        if (FD_ISSET (m_sockfd, &wSet))
        {
			if (sendData())  // 发送数据
			{
                clearSend(); // 整理发送缓存区
    			sendHeartBeat();  // 判断是否需要向服务器发送心跳
			}
		}
	}  // end while
}
/* 服务器心跳检查 */
void Client::heartBeatCheck (void) throw (ClientException)
{
    time_t now = time (NULL);  // 获取系统当前时间
    if (now - m_lastRecvHeartBeat < 3*HEARTRATE)  // 检查服务器心率
    {
        return;  // 服务器心跳正常
    }
    else
    {
        // 服务器异常，抛异常
        throw SocketException (strerror (errno));
    }
}
/* 接收数据 */
bool Client::recvData (void) throw (RecvException)
{
    if (m_recvBuf.m_isHeaderFull)  // 判断包头是否接收完
	{
        // 非阻塞接收包体，接收完返回true，否则返回false
        return recvBody();
    }
	else  // 若未接收完包头
	{
		int dataLen;
        if (ioctl (m_sockfd, FIONREAD, &dataLen) < 0)  // 判断底层buf是否收到包头
            return false;
        if (dataLen < 20)  // 未收到20个字节的包头
			return false;

        int ret = recv (m_sockfd, &m_recvBuf.m_recvBuf, 20, 0);  // 接收包头
        if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
        {
            errnos(); // 错误处理
            return false;
        }
        if (0 == ret) // 服务器关闭
 	    {
            cout << endl << "连接关闭！" << endl;
			throw RecvException (strerror (errno));
	    }

        //cout << "接收包头完成！　" << endl;
        m_recvBuf.m_curSize = 20;
        memcpy(&m_recvBuf.m_allSize, m_recvBuf.m_recvBuf+8, 4);  // 确定包的长度
        m_recvBuf.m_isHeaderFull = true;
        if (m_recvBuf.m_allSize == 20)
        {
            return true;
        }
	}
	return false;
}
/* 接收包体 */
bool Client::recvBody (void) throw (RecvException)
{
    int ret = recv (m_sockfd, m_recvBuf.m_recvBuf + m_recvBuf.m_curSize, m_recvBuf.m_allSize - m_recvBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞接收
    if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
    {
        errnos(); // 错误处理
        return false;
    }
    if (0 == ret) // 服务器关闭
	{
        cout << endl << "连接关闭！" << endl;
		throw RecvException (strerror (errno));
	}

    m_recvBuf.m_curSize += ret;  // 当前接收长度 = 原接收长度 + 实际接收长度
    if (m_recvBuf.m_allSize == m_recvBuf.m_curSize)  // 判断是否接收完
	{
        //cout << "接收包体完成！" << endl;
		return true;
	}
    else  // 未接收完
	{
		return false;
	}
}
/* 命令处理 */
void Client::commDeal (void) throw (ClientException)
{
	int commID;
	memcpy(&commID, m_recvBuf.m_recvBuf+4, 4);  // 提取commID
    if (commID == logIn_SUCCESS)  // 登录成功
	{
        cout <<  "logIn_SUCCESS" << endl;
        memcpy (&m_userID, m_recvBuf.m_recvBuf+12, 4);  // 提取m_userID
		cout << "my userID: " << m_userID << endl;  // 提示m_userID
		clearRecv ();  // 整理接收缓存区
	}
	else if (commID == pChat)  // 私聊
	{
		int userID;
        memcpy (&userID, m_recvBuf.m_recvBuf+12, 4);  // 提取userID
		cout << userID << " to me:" << endl;
		char msg[m_recvBuf.m_allSize-20+1] = { 0 };
        memcpy (msg, m_recvBuf.m_recvBuf+20, m_recvBuf.m_allSize-20);  // 提取消息
		cout << msg << endl;  // 提示消息
		clearRecv ();  // 整理接收缓存区
	}
	else if (commID == gChat)  // 群聊
	{
		int userID;
        memcpy (&userID, m_recvBuf.m_recvBuf+12, 4);  // 提取userID
		cout << userID << " to all:" << endl;
		char msg[m_recvBuf.m_allSize-20+1] = { 0 };
        memcpy (msg, m_recvBuf.m_recvBuf+20, m_recvBuf.m_allSize-20);  // 提取消息
		cout << msg << endl;  // 提示消息
		clearRecv ();  // 整理接收缓存区
	}
	else if (commID == pChat_FAILURE) // 私聊失败
	{
		cout << "私聊失败，用户不存在或已下线！" << endl;
        clearRecv();  // 整理接收缓存区
	}
	else if (commID == heartBeat) // 心跳包
	{
        cout << "server heartBeat" << endl;
		m_lastRecvHeartBeat = time (NULL);  // 服务器上次心跳时间
        clearRecv();  // 整理接收缓存区
	}
}
/* 整理接收缓存区 */
void Client::clearRecv (void) throw (ClientException)
{
    //cout << "清理接收缓存区..." << endl;
    m_recvBuf.m_curSize = 0;
    m_recvBuf.m_allSize = 0;
    m_recvBuf.m_isHeaderFull = false;
    memset (m_recvBuf.m_recvBuf, 0, sizeof (m_recvBuf.m_recvBuf));
}
/* 发送数据 */
bool Client::sendData (void) throw (SendException)
{
    int ret = send (m_sockfd, m_sendBuf.m_sendBuf + m_sendBuf.m_curSize, m_sendBuf.m_allSize - m_sendBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞发送
    if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
    {
        errnos(); // 错误处理
        return false;
    }
    if (0 == ret) // 服务器关闭
    {
        cout << endl << "连接关闭！" << endl;
        throw SendException (strerror (errno));
    }

    m_sendBuf.m_curSize += ret;  // 当前发送长度 = 原发送长度 + 实际发送长度
    if (m_sendBuf.m_allSize == m_sendBuf.m_curSize)  // 判断是否发送完
    {
        //cout << "发送数据完成！" << endl;
        return true;
	}
    else
    {
        // 未发完
		return false;
	}
}
/* 整理发送缓存区 */
void Client::clearSend (void) throw (ClientException)
{
    //cout << "清理发送缓存区..." << endl;
    m_sendBuf.m_allSize = 0;
    m_sendBuf.m_curSize = 0;
    memset (m_sendBuf.m_sendBuf, 0, sizeof (m_sendBuf.m_sendBuf));
}
/* 向服务器发送心跳 */
void Client::sendHeartBeat (void) throw (SendException)
{
    time_t now = time (NULL);  // 获取系统当前时间
    // 判断是否到发送心跳时间
    if (now - m_lastSendHeartBeat < HEARTRATE)
    {
        // 未到发送心跳时间
        return;
    }
    else
    {
        cout << "向服务器发送心跳！" << endl;
        // 向服务器发送心跳
        HEADER header;
        memset (&header, 0, sizeof (header));
        header.version = 4;
        header.commID = heartBeat;
        header.length = 20;
        header.userID = m_userID;
        header.to_userID = m_userID;

        int ret = send (m_sockfd, &header, 20, 0);
        if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
        {
            errnos(); // 错误处理
            return;
        }
        else if (0 == ret)  // 服务器关闭
        {
            cout << endl << "连接关闭！" << endl;
            throw SendException (strerror (errno));
        }
        else
        {
            // 发送心跳成功，更新上次发送心跳时间
            m_lastSendHeartBeat = now;
        }
    }
}
/* 启动指令线程 */
void Client::start (void) throw (ThreadException)
{
	cout << "启动指令线程开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
	cout << "启动指令线程完成！ " << endl;
}
/* 指令线程过程 */
void* Client::run (void* arg)
{
	pthread_detach (pthread_self());
    return static_cast<Client*> (arg)->loopComm();
}
/* 指令线程处理 */
void* Client::loopComm (void)
{
	while (1)
	{
        cout << endl << "指令提示：0.logIn 1.pChat 2.gChat 3.logOut" << endl;

        int commID;
        cin >> commID;  // 输入指令
        switch (commID)
        {
        case logIn:
            logInDeal();  // 登录指令
            break;
        case pChat:
            pChatDeal();  // 私聊指令
            break;
        case gChat:
            gChatDeal();  // 群聊指令
            break;
        case logOut:
            logOutDeal(); // 登出指令
            break;
        default:
            cout << "指令错误！请重新输入！" << endl;
            break;
        }  // end switch

        sleep (1);
    }  // end while
}
/* 登录指令处理 */
void Client::logInDeal (void)
{
    if (m_userID > 0)
    {
        cout << "你已登录！请重新输入指令！" << endl;
        return;
    }
    HEADER header;
    memset (&header, 0, 20);
    header.version = 4;    // 版本
    header.commID = logIn; // 登录指令
    header.length = 60;    // 包的长度
    header.userID = -1;    // 无效ID
    header.to_userID = -1; // 无效ID
    char userName[20] = { 0 }; // 用户名
    char userPWD[20] = { 0 };  // 密码
    cout << "请输入用户名和密码:" << endl;
    cin >> userName >> userPWD;
    memcpy (m_sendBuf.m_sendBuf, &header, 20);
    memcpy (m_sendBuf.m_sendBuf+20, userName, 20);
    memcpy (m_sendBuf.m_sendBuf+40, userPWD, 20);
    m_sendBuf.m_allSize = header.length;
}
/* 私聊指令处理 */
void Client::pChatDeal (void)
{
    if (m_userID < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    memset (&header, 0, 20);
    header.version = 4;    // 版本
    header.commID = pChat; // 私聊指令
    header.length = 20+1024;  // 包的长度
    header.userID = m_userID; // 用户ID
    int to_userID;
    cout << "请输入私聊用户ID:" << endl;
    cin >> to_userID;
    header.to_userID = to_userID;  // 私聊用户ID
    char msg_buf[1024];
    cout << "请输入私聊消息:" << endl;
    cin >> msg_buf;
    memcpy (m_sendBuf.m_sendBuf, &header, 20);
    memcpy (m_sendBuf.m_sendBuf+20, msg_buf, 1024);
    m_sendBuf.m_allSize = header.length;
}
/* 群聊指令处理 */
void Client::gChatDeal (void)
{
    if (m_userID < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    memset (&header, 0, 20);
    header.version = 4;    // 版本
    header.commID = gChat; // 群聊指令
    header.length = 20+1024;  // 包的长度
    header.userID = m_userID; // 用户ID
    header.to_userID = 0;  // 群聊用户ID
    char msg_buf[1024];
    cout << "请输入群聊消息:" << endl;
    cin >> msg_buf;
    memcpy (m_sendBuf.m_sendBuf, &header, 20);
    memcpy (m_sendBuf.m_sendBuf+20, msg_buf, 1024);
    m_sendBuf.m_allSize = header.length;
}
/* 登出指令处理 */
void Client::logOutDeal (void)
{
    if (m_userID < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    memset (&header, 0, 20);
    header.version = 4;     // 版本
    header.commID = logOut; // 私聊指令
    header.length = 20;     // 包的长度
    header.userID = m_userID; // 用户ID
    header.to_userID = -1;    // 无效ID
    memcpy (m_sendBuf.m_sendBuf, &header, 20);
    m_sendBuf.m_allSize = header.length;
}
