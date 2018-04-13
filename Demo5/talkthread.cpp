/* 实现会话线程类 */ 
#include <sys/select.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h> 
#include <time.h>
#include <sys/epoll.h>
#include <sys/socket.h>    
#include <sys/ioctl.h>
#include "talkthread.h"
#include "addqueue.h"
#include "cmdqueue.h"
#include "errnos.h"
/* 启动会话线程 */
void CTalkThread::start (void) throw (ThreadException) 
{
    cout << "启动数据I/O模块开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
    cout << "启动数据I/O模块完成！ " << endl;
}
/* 会话线程过程 */
void* CTalkThread::run (void* arg) 
{
	pthread_detach (pthread_self());
	return static_cast<CTalkThread*> (arg)->loopTalk();
}
/* 会话线程处理 */
void* CTalkThread::loopTalk (void) 
{
    /* 声明epoll_event结构体的变量，ev用于注册事件，数组用于回传要处理的事件 */
    struct epoll_event ev, events[MAXEVENTS];
    m_epollfd = epoll_create (MAXUSERS);

	while (1) 
	{
        while (!g_addQueue.empty())  // 判断新增用户队列是否为空
        {
            // 获取新增用户队列首元素
            CUser* pUser = g_addQueue.frontAdd();
            // 删除新增用户队列首元素
            g_addQueue.popAdd();
            // 添加到临时映射
            m_userMap.insert (pair<int, CUser*> (pUser->getUserID(), pUser));
            // 设置关联相应用户
            ev.data.ptr = pUser;
            // 设置相应套接字可读，默认水平触发模式
            ev.events = EPOLLIN;
            // 注册epoll事件
            epoll_ctl (m_epollfd, EPOLL_CTL_ADD, pUser->getUserFd(), &ev);
        }  // end while

        for (map<int, CUser*>::iterator it = m_userMap.begin(); it != m_userMap.end(); ++it)
        {
            // 判断是否当前用户待删除
            if (it->second->getDelete() == 1)
            {
                // 删除epoll事件
                epoll_ctl (m_epollfd, EPOLL_CTL_DEL, it->second->getUserFd(), NULL);
                markDelete (it->second);  // 标记可删除用户
                map<int, CUser*>::iterator temp = it;
                m_userMap.erase (temp);  // 解除映射
            }
            // 判断是否有未发完的数据
            else if (it->second->m_sendBuf.m_allSize != it->second->m_sendBuf.m_curSize)
            {
                // 设置关联相应用户
                ev.data.ptr = it->second;
                // 设置相应套接字可读可写，默认水平触发模式
                ev.events = EPOLLIN | EPOLLOUT;
                // 修改epoll事件
                epoll_ctl (m_epollfd, EPOLL_CTL_MOD, it->second->getUserFd(), &ev);
            }
            // 判断是否需要向客户端发送心跳
            else
            {
                sendHeartBeat (it->second);
            }
        }  // end for

        // 等待epoll事件的发生
        int nReady = epoll_wait (m_epollfd, events, MAXEVENTS, 5);
        if (0 == nReady)  // 超时
            continue;

        // 处理发生的所有事件
        for (int i = 0; i < nReady; ++i)
        {
            // 判断是否有可读事件
            if (events[i].events & EPOLLIN)
            {
                CUser* pUser = (CUser*)events[i].data.ptr;
                if (recvData (pUser->getUserID(), pUser))  // 接收数据
                {
                    HEADER* pHeader = new HEADER;
                    memcpy (&pHeader->version, pUser->m_recvBuf.m_recvBuf, 4);
                    memcpy (&pHeader->commID, pUser->m_recvBuf.m_recvBuf+4, 4);
                    memcpy (&pHeader->length, pUser->m_recvBuf.m_recvBuf+8, 4);
                    memcpy (&pHeader->userID, pUser->m_recvBuf.m_recvBuf+12, 4);
                    if (pHeader->userID < 0)
                    {
                        // 若为logIn指令，将userID放到pHeader中
                        int userID = pUser->getUserID();
                        memcpy (&pHeader->userID, &userID, 4);
                    }
                    g_cmdQueue.pushCmd (pHeader);  // 向上抛出命令
                }
            }
            // 判断是否有可写事件
            if (events[i].events & EPOLLOUT)
            {
                CUser* pUser = (CUser*)events[i].data.ptr;
                if (sendData (pUser->getUserID(), pUser))  // 发送数据
                {
                    clearSend (pUser);  // 清理发送缓存区
                    // 设置关联相应套接字
                    ev.data.fd = pUser->getUserFd();
                    // 设置关联相应用户
                    ev.data.ptr = pUser;
                    // 设置相应套接字只读，默认水平触发模式
                    ev.events = EPOLLIN;
                    // 修改epoll事件
                    epoll_ctl (m_epollfd, EPOLL_CTL_MOD, pUser->getUserFd(), &ev);
                }
            }
        }  // end for
	}  // end while
}
/* 向客户端发送心跳 */
void CTalkThread::sendHeartBeat (CUser* pUser) throw (SendException)
{
    time_t now = time (NULL);  // 获取系统当前时间
    // 判断是否到发送心跳时间
    if (now - pUser->m_lastSendHeartBeat < HEARTRATE)
    {
        // 未到发送心跳时间
        return;
    }
    else
    {
        cout << "向客户端发送心跳！" << endl;
        // 向客户端发送心跳
        HEADER header;
        memset (&header, 0, sizeof (header));
        header.version = VERSION;
        header.commID = heartBeat;
        header.length = sizeof (header)+4;
        header.userID = pUser->getUserID();
        int to_userID = pUser->getUserID();

        memcpy (pUser->m_sendBuf.m_sendBuf, &header, sizeof (header));
        memcpy (pUser->m_sendBuf.m_sendBuf+sizeof (header), &to_userID, 4);
        pUser->m_sendBuf.m_allSize = header.length;  // 确定包的长度

        // 声明epoll_event结构体的变量用于修改事件
        struct epoll_event ev;
        // 设置关联相应用户
        ev.data.ptr = pUser;
        // 设置相应套接字可读可写，默认水平触发模式
        ev.events = EPOLLIN | EPOLLOUT;
        // 修改epoll事件
        epoll_ctl (m_epollfd, EPOLL_CTL_MOD, pUser->getUserFd(), &ev);
        // 更新上次发送心跳时间
        pUser->m_lastSendHeartBeat = now;
    }
}
/* 接收数据 */
bool CTalkThread::recvData (int userID, CUser* pUser) throw (RecvException)
{
    if (pUser->m_recvBuf.m_isHeaderFull)  // 判断包头是否接收完
	{
        // 非阻塞接收包体，接收完返回true，否则返回false
        return recvBody (userID, pUser);
    }
	else  // 若未接收完包头
	{
		int dataLen;
        if (ioctl (pUser->getUserFd(), FIONREAD, &dataLen) < 0)  // 判断底层buf是否收到包头
            return false;
        if (dataLen < 16)  // 未收到16个字节的包头
			return false;

        int ret = recv (pUser->getUserFd(), &pUser->m_recvBuf.m_recvBuf, 16, 0);  // 接收包头
        if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
        {
            errnos(); // 错误处理
            return false;
        }
        if (0 == ret) // 客户端关闭
        {
            // 删除epoll事件
            epoll_ctl (m_epollfd, EPOLL_CTL_DEL, pUser->getUserFd(), NULL);
            markDelete (pUser);  // 标记可删除用户
            m_userMap.erase (userID);  // 解除映射
			return false;
        }

        cout << "接收包头完成！" << endl;
        pUser->m_recvBuf.m_curSize = 16;
        memcpy(&pUser->m_recvBuf.m_allSize, pUser->m_recvBuf.m_recvBuf+8, 4);  // 确定包的总长度
        pUser->m_recvBuf.m_isHeaderFull = true;

        if (pUser->m_recvBuf.m_allSize == pUser->m_recvBuf.m_curSize)
        {
            return true;
        }
	}
	return false;
}
/* 接收包体 */
bool CTalkThread::recvBody (int userID, CUser* pUser) throw (RecvException)
{
    int ret = recv (pUser->getUserFd(), pUser->m_recvBuf.m_recvBuf + pUser->m_recvBuf.m_curSize, pUser->m_recvBuf.m_allSize - pUser->m_recvBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞接收
    if (ret < 0) // 失败返回-1，errno被设为以下的某个值
    {
        errnos();  // 错误处理
        return false;
    }
    if (0 == ret) // 客户端关闭
    {
        // 删除epoll事件
        epoll_ctl (m_epollfd, EPOLL_CTL_DEL, pUser->getUserFd(), NULL);
        markDelete (pUser);  // 标记可删除用户
        m_userMap.erase (userID);  // 解除映射
        return false;
    }

    pUser->m_recvBuf.m_curSize += ret;  // 当前接收长度 = 原接收长度 + 实际接收长度
    if (pUser->m_recvBuf.m_allSize == pUser->m_recvBuf.m_curSize)  // 判断是否接收完
	{
        cout << "接收包体完成！" << endl;
		return true;
	}
    else  // 未接收完
	{
		return false;
	}
}
/* 发送数据 */
bool CTalkThread::sendData (int userID, CUser* pUser) throw (SendException)
{
    if (memcmp (&pUser->m_userID, pUser->m_sendBuf.m_sendBuf+16, 4) != 0)  // 废包处理
    {
        cout << "废包，已忽略！" << endl;
        return true;
    }

    int ret = send (pUser->getUserFd(), pUser->m_sendBuf.m_sendBuf + pUser->m_sendBuf.m_curSize, pUser->m_sendBuf.m_allSize - pUser->m_sendBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞发送
    if (ret < 0) // 失败返回-1，errno被设为以下的某个值
    {
        errnos();  // 错误处理
        return false;
    }
    if (0 == ret)  // 客户端关闭
    {
        // 删除epoll事件
        epoll_ctl (m_epollfd, EPOLL_CTL_DEL, pUser->getUserFd(), NULL);
        markDelete (pUser);  // 标记可删除用户
        m_userMap.erase (userID);  // 解除映射
        return false;
    }

    pUser->m_sendBuf.m_curSize += ret;  // 当前发送长度 = 原发送长度 + 实际发送长度
    if (pUser->m_sendBuf.m_allSize == pUser->m_sendBuf.m_curSize)  // 判断是否发送完
    {
        cout << "发送数据完成！" << endl;
        return true;
	}
    else
    {
        // 未发完
		return false;
	}
}
/* 整理发送缓存区 */
void CTalkThread::clearSend (CUser* pUser) throw (ThreadException)
{
    cout << "清理发送缓存区..." << endl;
    pUser->m_sendBuf.m_curSize = 0;
    pUser->m_sendBuf.m_allSize = 0;
    memset (pUser->m_sendBuf.m_sendBuf, 0, sizeof (pUser->m_sendBuf.m_sendBuf));
}
/* 标记可删除用户 */
void CTalkThread::markDelete (CUser* pUser)
{
    pUser->setDelete (2);
}
