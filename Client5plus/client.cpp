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
#include "packet.h"
/* 构造器 */
Client::Client (short port, string const& ip/* = "127.0.0.1"*/)
	throw (ClientException): m_port(port), m_ip(ip) 
{
	m_lastSendHeartBeat = time (NULL);  // 上次发送心跳时间
	m_lastRecvHeartBeat = time (NULL);  // 服务器上次心跳时间
    m_userID = -1;

    m_fileSendYes = false;
    m_fileSend = 0;
    m_fileRecvID = -1;

    m_fileSize = -1;
    m_fileSendID = -1;
}
/* 析构器 */
Client::~Client (void)
{
    close (m_epollfd);
	close (m_sockfd);  // 关闭客户端套接字
    cout << "m_sockfd is closed..." << endl;
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

CONNECT:
    // 向服务器发起连接请求，三次握手
	int ret = connect (m_sockfd, (struct sockaddr*)&servAddr, sizeof (servAddr));


    // m_sockfd若为非阻塞，connect函数调用后，无论是否建立连接，都会立即返回-1
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
            FD_SET (m_sockfd, &rSet);
            FD_SET (m_sockfd, &wSet);

            // 5ms
            timeout.tv_sec  = 0;     // 秒
            timeout.tv_usec = 5000;  // 毫秒

            int nReady = select (m_sockfd + 1, &rSet, &wSet, NULL, &timeout);
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
                getsockopt (m_sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&len);
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
    /* 声明epoll_event结构体的变量，ev用于注册事件，数组用于回传要处理的事件 */
    struct epoll_event ev, events[MAXEVENTS];
    m_epollfd = epoll_create (MAXUSERS);

    // 设置套接字描述符可读，默认水平触发模式
    ev.events = EPOLLIN;
    // 注册epoll事件
    epoll_ctl (m_epollfd, EPOLL_CTL_ADD, m_sockfd, &ev);

	// 循环监控
    while (1)
    {
        // 服务器心跳检查
        heartBeatCheck();

		// 判断是否有未发完的数据
        if (m_sendBuf.m_allSize != m_sendBuf.m_curSize)
		{
            // 设置套接字描述符可读可写，默认水平触发模式
            ev.events = EPOLLIN | EPOLLOUT;
            // 修改epoll事件
            epoll_ctl (m_epollfd, EPOLL_CTL_MOD, m_sockfd, &ev);
		}
        else
		{
            if (m_fileSendYes)
            {
                fileSend();  // 发送文件

                ev.events = EPOLLIN | EPOLLOUT;
                epoll_ctl (m_epollfd, EPOLL_CTL_MOD, m_sockfd, &ev);
            }
            else
            {
                // 判断是否需要向服务器发送心跳
                sendHeartBeat();
            }
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
                if (recvData())  // 接收数据
                {
                    commDeal();  // 命令处理
                }
            }

            // 判断是否有可写事件
            if (events[i].events & EPOLLOUT)
            {
                if (sendData())  // 发送数据
                {
                    clearSend(); // 清理发送缓存区

                    // 设置套接字描述符只读，默认水平触发模式
                    ev.events = EPOLLIN;
                    // 修改epoll事件
                    epoll_ctl (m_epollfd, EPOLL_CTL_MOD, m_sockfd, &ev);
                }
            }
        }  // end for
	}  // end while
}
/* 发送文件 */
void Client::fileSend (void)
{
    if (m_filePath[0] == '\0')
    {
        cout << "文件传输完成！！！" << endl;
        m_fileSendYes = false;
    }
    else
    {
        char buf[1300] = { 0 };
        FILE* fp = fopen (m_filePath, "rb");  // 只读方式打开二进制文件
        if (fp < 0)
        {
            cout << "无此文件！" << endl;
            memset (m_filePath, 0, sizeof (m_filePath));
            m_fileSend = 0;
            m_fileRecvID = -1;
            m_fileSendYes = false;
        }
        else
        {
            fseek (fp, m_fileSend, SEEK_SET);  // 设置到当前发送长度位置，进行读取
            int ret = fread (buf, 1, sizeof (buf), fp);  // 读取数据到buf
            fclose (fp);  // 关闭文件描述符

            HEADER header;
            header.version = VERSION;
            header.commID = transFile_ING;
            header.length = sizeof (header)+sizeof (m_fileRecvID)+ret;
            header.userID = m_userID;

            memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
            memcpy (m_sendBuf.m_sendBuf+sizeof (header), &m_fileRecvID, sizeof (m_fileRecvID));
            memcpy (m_sendBuf.m_sendBuf+sizeof (header)+sizeof (m_fileRecvID), buf, ret);
            m_sendBuf.m_allSize = header.length;  // 确定包的长度

            m_fileSend += ret;  // 保存当前发送长度
            cout << "send " << m_fileSend << endl;
            struct stat st;
            stat (m_filePath, &st);  // 获取文件大小
            if (m_fileSend == st.st_size)
            {
                cout << "文件传输完成！" << endl;
                memset (m_filePath, 0, sizeof (m_filePath));
                m_fileSend = 0;
                m_fileRecvID = -1;
                m_fileSendYes = false;
            }
        }  // end else
    }  // end else
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
        // 服务器异常，做相关操作，比如重连等，return
    }
}
/* 接收数据 */
bool Client::recvData (void) throw (RecvException)
{
    //cout << "接收数据开始..." << endl;
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
        if (dataLen < 16)  // 未收到16个字节的包头
			return false;

        int ret = recv (m_sockfd, &m_recvBuf.m_recvBuf, 16, 0);  // 接收包头
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
        m_recvBuf.m_curSize = 16;
        memcpy (&m_recvBuf.m_allSize, m_recvBuf.m_recvBuf+8, 4);  // 确定包的长度
        m_recvBuf.m_isHeaderFull = true;
	}
	return false;
}
/* 接收包体 */
bool Client::recvBody (void) throw (RecvException)
{
    cout << "接收包体开始..." << endl;
    int ret = recv (m_sockfd, m_recvBuf.m_recvBuf + m_recvBuf.m_curSize, m_recvBuf.m_allSize - m_recvBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞接收
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

    m_recvBuf.m_curSize += ret;  // 当前接收长度 = 原接收长度 + 实际接收长度
    if (m_recvBuf.m_allSize == m_recvBuf.m_curSize)  // 判断是否接收完
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
/* 指令处理 */
void Client::commDeal (void) throw (ClientException)
{
    cout << "指令处理开始..." << endl;
	int commID;
    memcpy (&commID, m_recvBuf.m_recvBuf+4, 4);  // 提取commID

    if (commID == logIn_SUCCESS)  // 登录成功
	{
        cout << "logIn_SUCCESS" << endl;
        memcpy (&m_userID, m_recvBuf.m_recvBuf+12, 4);  // 提取用户ID
        cout << "my userID: " << m_userID << endl;  // 提示用户ID
	}
	else if (commID == pChat)  // 私聊
	{
        cout << "pChat" << endl;
		int userID;
        memcpy (&userID, m_recvBuf.m_recvBuf+12, 4);  // 提取userID
		cout << userID << " to me:" << endl;

        char msg[1024+1] = { 0 };
        memcpy (msg, m_recvBuf.m_recvBuf+20, m_recvBuf.m_allSize-20);  // 提取消息
		cout << msg << endl;  // 提示消息
	}
	else if (commID == gChat)  // 群聊
	{
        cout << "gChat" << endl;
		int userID;
        memcpy (&userID, m_recvBuf.m_recvBuf+12, 4);  // 提取userID
		cout << userID << " to all:" << endl;

        char msg[1024+1] = { 0 };
        memcpy (msg, m_recvBuf.m_recvBuf+20, m_recvBuf.m_allSize-20);  // 提取消息
		cout << msg << endl;  // 提示消息
	}
    else if (commID == pChat_FAILURE)  // 私聊失败
	{
        cout << "pChat_FAILURE" << endl;
		cout << "私聊失败，用户不存在或已下线！" << endl;
	}
    else if (commID == transFile)  // 传输文件
	{
        cout <<  "transFile" << endl;
        memcpy (&m_fileSendID, m_recvBuf.m_recvBuf+12, sizeof (m_fileSendID)); // 提取userID
        memcpy (m_fileName, m_recvBuf.m_recvBuf+20, sizeof (m_fileName));      // 提取文件名
        memcpy (&m_fileSize, m_recvBuf.m_recvBuf+40, sizeof (m_fileSize));     // 提取文件大小
        cout << m_fileSendID << " to me: " << m_fileName << ' ' << m_fileSize << endl;
        cout << "接受还是拒绝？(y/n)(10/11)" << endl;
	}
    else if (commID == transFile_SENDFAIL)  // 传输文件失败
	{
        cout << "transFile_SENDFAIL" << endl;
        cout << "传输文件失败，用户不存在或已下线！" << endl;
		memset (m_filePath, 0, sizeof (m_filePath));
        m_fileSend = 0;
        m_fileRecvID = -1;
        m_fileSendYes = false;
	}
    else if (commID == transFile_RECVFAIL)  // 接收文件失败
    {
        cout << "transFile_RECVFAIL" << endl;
        cout << "接收文件失败，用户不存在或已下线！" << endl;
        memset (m_fileName, 0, sizeof (m_fileName));
        m_fileSize = -1;
        m_fileSendID = -1;
    }
    else if (commID == transFile_ING)  // 传输文件中
	{
        cout << "transFile_ING" << endl;
        if (m_fileSize < 0)
        {
            cout << "文件接收完成！！！" << endl;
        }
        else
        {
            HEADER header;
            memcpy (&header, m_recvBuf.m_recvBuf, sizeof (header));
            FILE* fp = fopen (m_fileName, "ab");  // 以只写的二进制方式追加，无则创建
            fwrite (m_recvBuf.m_recvBuf+20, header.length-20, 1, fp);  // 从接收缓存区追加到文件
            fclose (fp);  // 关闭文件描述符

            struct stat st;
            stat (m_fileName, &st);  // 获取文件大小
            cout << "recv " << st.st_size << endl;
            if (st.st_size == m_fileSize)
            {
                cout << "文件接收完成！" << endl;
                memset (m_fileName, 0, sizeof (m_fileName));
                m_fileSize = -1;
                m_fileSendID = -1;
            }
        }

        m_lastRecvHeartBeat = time (NULL);  // 更新服务器上次心跳时间
	}
    else if (commID == transFile_YES)  // 接受传输文件
	{
        cout << "transFile_YES" << endl;
        cout << "目标用户接受传输文件！" << endl;
        m_fileSendYes = true;
	}
    else if (commID == transFile_NO)  // 拒绝传输文件
	{
        cout << "transFile_NO" << endl;
		cout << "目标用户拒绝传输文件！" << endl;
		memset (m_filePath, 0, sizeof (m_filePath));
        m_fileSend = 0;
        m_fileRecvID = -1;
        m_fileSendYes = false;
	}
    else if (commID == heartBeat)  // 心跳包
	{
        cout << "server heartBeat" << endl;
        m_lastRecvHeartBeat = time (NULL);  // 更新服务器上次心跳时间
	}

    cout << "指令处理完成！" << endl;
    clearRecv();  // 整理接收缓存区
}
/* 整理接收缓存区 */
void Client::clearRecv (void) throw (ClientException)
{
    cout << "清理接收缓存区开始..." << endl;
    m_recvBuf.m_curSize = 0;
    m_recvBuf.m_allSize = 0;
    m_recvBuf.m_isHeaderFull = false;
    memset (m_recvBuf.m_recvBuf, 0, sizeof (m_recvBuf.m_recvBuf));
    cout << "清理接收缓存区完成！" << endl;
}
/* 发送数据 */
bool Client::sendData (void) throw (SendException)
{
    usleep (1000);  // 1ms/1min40s
    cout << "发送数据开始..." << endl;
    int ret = send (m_sockfd, m_sendBuf.m_sendBuf + m_sendBuf.m_curSize, m_sendBuf.m_allSize - m_sendBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞发送
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

    m_sendBuf.m_curSize += ret;  // 当前发送长度 = 原发送长度 + 实际发送长度
    if (m_sendBuf.m_allSize == m_sendBuf.m_curSize)  // 判断是否发送完
    {
        cout << "发送数据完成！" << endl;
        return true;
	}
    else
    {
        cout << "发送数据未完成..." << endl;
		return false;
	}
}
/* 整理发送缓存区 */
void Client::clearSend (void) throw (ClientException)
{
    cout << "清理发送缓存区开始..." << endl;
    m_sendBuf.m_allSize = 0;
    m_sendBuf.m_curSize = 0;
    memset (m_sendBuf.m_sendBuf, 0, sizeof (m_sendBuf.m_sendBuf));
    cout << "清理发送缓存区完成！" << endl;
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
        if (m_userID < 0)  // 未登录，无需发心跳
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
        epoll_ctl (m_epollfd, EPOLL_CTL_MOD, m_sockfd, &ev);

        // 更新上次发送心跳时间
        m_lastSendHeartBeat = now;
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
        cout << endl << "指令提示：0.logIn 1.pChat 2.gChat 3.transFile 4.logOut" << endl;

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
        case transFile:
            transFileDeal();  // 传输文件指令
            break;
        case transFile_YES:
			yesDeal(); // 接受传输文件指令
			break;
        case transFile_NO:
			noDeal();  // 拒绝传输文件指令
            break;
        case logOut:
            logOutDeal(); // 登出指令
            break;
        default:
            cout << "指令错误！请重新输入！" << endl;
            break;
        }  // end switch

        usleep (500000);  // 500ms
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
    header.version = VERSION;           // 版本
    header.commID = logIn;              // 登录指令
    header.length = sizeof (header)+40; // 包的长度
    header.userID = -1;                 // 无效ID

    char userName[20] = { 0 }; // 用户名
    char userPWD[20] = { 0 };  // 密码
    cout << "请输入用户名和密码:" << endl;
    cin >> userName >> userPWD;

    memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header), userName, sizeof (userName));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header)+sizeof (userName), userPWD, sizeof (userPWD));
    m_sendBuf.m_allSize = header.length;  // 确定包的长度
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
    header.version = VERSION;               // 版本
    header.commID = pChat;                  // 私聊指令
    header.length = sizeof (header)+4+1024; // 包的长度
    header.userID = m_userID;               // 用户ID

    int to_userID;
    cout << "请输入私聊用户ID:" << endl;
    cin >> to_userID;
    char msg_buf[1024];
    cout << "请输入私聊消息:" << endl;
    cin >> msg_buf;

    memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header), &to_userID, sizeof (to_userID));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header)+sizeof (to_userID), msg_buf, sizeof (msg_buf));
    m_sendBuf.m_allSize = header.length;  // 确定包的长度
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
    memset (&header, 0, sizeof (header));
    header.version = VERSION;             // 版本
    header.commID = gChat;                // 群聊指令
    header.length = sizeof (header)+1024; // 包的长度
    header.userID = m_userID;             // 用户ID

    char msg_buf[1024];
    cout << "请输入群聊消息:" << endl;
    cin >> msg_buf;

    memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header), msg_buf, sizeof (msg_buf));
    m_sendBuf.m_allSize = header.length;  // 确定包的长度
}
/* 传输文件指令处理 */
void Client::transFileDeal (void)
{
    if (m_userID < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    memset (&header, 0, sizeof (header));
    header.version = VERSION;  // 版本
    header.commID = transFile; // 传输文件指令
    header.userID = m_userID;  // 用户ID

    cout << "请输入目标用户ID:" << endl;
    cin >> m_fileRecvID;

    memset (m_filePath, 0, sizeof (m_filePath));
    cout << "请输入文件路径:" << endl;
    cin >> m_filePath;

	char fileName[20] = { 0 };
    char* tmp = basename (m_filePath);
    memcpy (fileName, tmp, 20);  // 得到文件名
	cout << "文件名 " << fileName << endl;

	struct stat st;
    if (stat (m_filePath, &st) < 0)
    {
        cout << "无此文件！" << endl;
        memset (m_filePath, 0, sizeof (m_filePath));
        m_fileSend = 0;
        m_fileRecvID = -1;
        return;
    }
    cout << "文件大小 " << st.st_size << endl;  // 得到文件大小

    header.length = sizeof (header)+sizeof (m_fileRecvID)+sizeof (fileName)+sizeof (st.st_size); // 包的长度
	memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header), &m_fileRecvID, sizeof (m_fileRecvID));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header)+sizeof (m_fileRecvID), fileName, sizeof (fileName));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header)+sizeof (m_fileRecvID)+sizeof (fileName), &st.st_size, sizeof (st.st_size));
	m_sendBuf.m_allSize = header.length;  // 确定包的长度
}
/* 接受传输文件指令处理 */
void Client::yesDeal (void)
{
    if (m_fileSize < 0)
	{
        cout << "指令错误！请重新输入！" << endl;
		return;
    }
    HEADER header;
    memset (&header, 0, sizeof (header));
    header.version = VERSION;      // 版本
    header.commID = transFile_YES; // 接受传输文件指令
    header.length = sizeof (header)+sizeof (m_fileSendID);  // 包的长度
    header.userID = m_userID;      // 用户ID

	memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header), &m_fileSendID, sizeof (m_fileSendID));
	m_sendBuf.m_allSize = header.length;  // 确定包的长度
}
/* 拒绝传输文件指令处理 */
void Client::noDeal (void)
{
    if (m_fileSize < 0)
	{
        cout << "指令错误！请重新输入！" << endl;
		return;
    }
    HEADER header;
    memset (&header, 0, sizeof (header));
    header.version = VERSION;     // 版本
    header.commID = transFile_NO; // 拒绝传输文件指令
    header.length = sizeof (header)+sizeof (m_fileSendID);  // 包的长度
    header.userID = m_userID;     // 用户ID

	memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (m_sendBuf.m_sendBuf+sizeof (header), &m_fileSendID, sizeof (m_fileSendID));
	m_sendBuf.m_allSize = header.length;  // 确定包的长度

    memset (m_fileName, 0, sizeof (m_fileName));
    m_fileSize = -1;
    m_fileSendID = -1;
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
    memset (&header, 0, sizeof (header));
    header.version = VERSION;        // 版本
    header.commID = logOut;          // 登出指令
    header.length = sizeof (header); // 包的长度
    header.userID = m_userID;        // 用户ID

    memcpy (m_sendBuf.m_sendBuf, &header, sizeof (header));
    m_sendBuf.m_allSize = header.length;  // 确定包的长度

    cout << "你已登出！" << endl;
}
