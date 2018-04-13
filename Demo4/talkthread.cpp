/* 实现会话线程类 */ 
#include <sys/select.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h> 
#include <time.h>
#include <sys/socket.h>    
#include <sys/ioctl.h>
#include "talkthread.h"
#include "queue.h"
#include "userqueue.h"
#include "errnos.h"
/* 启动会话线程 */
void CTalkThread::start (void) throw (ThreadException) 
{
	cout << "启动会话线程开始..." << endl;
	int error = pthread_create (&m_tid, NULL, run, this);
	if (error)
		throw ThreadException (strerror (errno));
	cout << "启动会话线程完成！ " << endl;
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
	fd_set rSet;  // 定义可读事件集合
	fd_set wSet;  // 定义可写事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */

	while (1) 
	{
        FD_ZERO (&rSet);  // 清空可读事件集合
        FD_ZERO (&wSet);  // 清空可写事件集合
        int max_fd = 0;   // 集合最大有效下标

        while (!g_queue.empty())  // 判断队列是否为空
        {
            g_userMap.insert (pair<int, CUser*> (g_queue.front()->m_userID, g_queue.front()));  // 添加到映射
            g_queue.pop();  // 删除队列首元素
        }  // end while

        for (map<int, CUser*>::iterator it = g_userMap.begin(); it != g_userMap.end(); ++it)
		{
            FD_SET (it->second->m_connfd, &rSet);  // 添加到可读事件集合

            // 判断是否有未发完的数据
            if (it->second->m_sendBuf.m_allSize != it->second->m_sendBuf.m_curSize)
            {
                cout << "有数据需要发送..." << endl;
                // 若有，添加到可写事件集合
                FD_SET (it->second->m_connfd, &wSet);
            }
            else
            {
                // 若没有，判断是否需要向客户端发送心跳
                sendHeartBeat (it->second);
            }

            if (it->second->m_connfd > max_fd)
                max_fd = it->second->m_connfd;  // 确定集合最大有效下标
        }  // end for

		timeout.tv_sec = 0;      // 秒
		timeout.tv_usec = 5000;  // 微秒
        int nReady = select (max_fd + 1, &rSet, &wSet, NULL, &timeout);
        if (nReady < 0) // 失败返回-1，errno被设为以下的某个值
        {
            errnos();  // 错误处理
            continue;  // 继续
        }
		if (0 == nReady)  // 超时
			continue;
			
        for (map<int, CUser*>::iterator it = g_userMap.begin(); it != g_userMap.end(); ++it)
		{
			// 判断是否有可读事件
            if (FD_ISSET (it->second->m_connfd, &rSet))
			{
                if (recvData (it->first, it->second))  // 接收数据
                {
                    g_userQueue.pushUser (it->second); // 向上抛出命令
                }
            }

			// 判断是否有可写事件
            if (FD_ISSET (it->second->m_connfd, &wSet))
			{
                if (sendData (it->first, it->second))  // 发送数据
                {
                    clearSend (it->second);  // 清理发送缓存区
                    sendHeartBeat (it->second);  // 判断是否需要向客户端发送心跳
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
        header.version = 4;
        header.commID = heartBeat;
        header.length = 20;
        header.userID = pUser->m_userID;
        header.to_userID = pUser->m_userID;

        int ret = send (pUser->getUserFd(), &header, 20, 0);
        if (ret < 0)
        {
            return;
        }
        else if (0 == ret)  // 客户端关闭
        {
            delUser (pUser);  // 删除用户
            return;
        }
        else
        {
            // 发送心跳成功，更新上次发送心跳时间
            pUser->m_lastSendHeartBeat = now;
        }
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
        if (ioctl (pUser->m_connfd, FIONREAD, &dataLen) < 0)  // 判断底层buf是否收到包头
            return false;
        if (dataLen < 20)  // 未收到20个字节的包头
			return false;

        int ret = recv (pUser->m_connfd, &pUser->m_recvBuf.m_recvBuf, 20, 0);  // 接收包头
        if (ret < 0)  // 失败返回-1，errno被设为以下的某个值
        {
            errnos(); // 错误处理
            return false;
        }
        if (0 == ret) // 客户端关闭
        {
            delUser (pUser);  // 删除用户
			return false;
        }
		// 接收完包头
        pUser->m_recvBuf.m_curSize = 20;
        memcpy(&pUser->m_recvBuf.m_allSize, pUser->m_recvBuf.m_recvBuf+8, 4);  // 确定包的总长度
        pUser->m_recvBuf.m_isHeaderFull = true;
        if (pUser->m_recvBuf.m_allSize == 20)
        {
            return true;
        }
	}
	return false;
}
/* 接收包体 */
bool CTalkThread::recvBody (int userID, CUser* pUser) throw (RecvException)
{
    int ret = recv (pUser->m_connfd, pUser->m_recvBuf.m_recvBuf + pUser->m_recvBuf.m_curSize, pUser->m_recvBuf.m_allSize - pUser->m_recvBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞接收
    if (ret < 0) // 失败返回-1，errno被设为以下的某个值
    {
        errnos();  // 错误处理
        return false;
    }
    if (0 == ret) // 客户端关闭
    {
        delUser (pUser);  // 删除用户
        return false;
    }

    pUser->m_recvBuf.m_curSize += ret;  // 当前接收长度 = 原接收长度 + 实际接收长度
    if (pUser->m_recvBuf.m_allSize == pUser->m_recvBuf.m_curSize)  // 判断是否接收完
	{
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

    int ret = send (pUser->m_connfd, pUser->m_sendBuf.m_sendBuf + pUser->m_sendBuf.m_curSize, pUser->m_sendBuf.m_allSize - pUser->m_sendBuf.m_curSize, MSG_DONTWAIT);  // 非阻塞发送
    if (ret < 0) // 失败返回-1，errno被设为以下的某个值
    {
        errnos();  // 错误处理
        return false;
    }
    if (0 == ret)  // 客户端关闭
    {
        delUser (pUser);  // 删除用户
        return false;
    }

    pUser->m_sendBuf.m_curSize += ret;  // 当前发送长度 = 原发送长度 + 实际发送长度
    if (pUser->m_sendBuf.m_allSize == pUser->m_sendBuf.m_curSize)  // 判断是否发送完
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
/* 整理发送缓存区 */
void CTalkThread::clearSend (CUser* pUser) throw (ThreadException)
{
    cout << "清理发送缓存区..." << endl;
    pUser->m_sendBuf.m_curSize = 0;
    pUser->m_sendBuf.m_allSize = 0;
    memset (pUser->m_sendBuf.m_sendBuf, 0, sizeof (pUser->m_sendBuf.m_sendBuf));
}

map<int, CUser*> g_userMap;  // 用户ID和用户类映射
/* 删除用户 */
void delUser (CUser* pUser)
{
    g_userMap.erase (pUser->m_userID);
    if (pUser != NULL)
    {
        delete pUser;  // 销毁用户
        pUser = NULL;  // 置空
    }
}
