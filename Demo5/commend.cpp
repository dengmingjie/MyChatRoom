/* 指令处理模块 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
using namespace std;
#include "commend.h"
#include "newqueue.h"
#include "addqueue.h"
#include "cmdqueue.h"
#include "packet.h"
/* 启动指令处理线程 */
void CCommendDeal::start (void) throw (ThreadException)
{
    cout << "启动指令处理模块开始..." << endl;
    int error = pthread_create (&m_tid, NULL, run, this);
    if (error)
        throw ThreadException (strerror (errno));
    cout << "启动指令处理模块完成！ " << endl;
}
/* 线程过程 */
void* CCommendDeal::run (void* arg)
{
    pthread_detach (pthread_self());
    return static_cast<CCommendDeal*> (arg)->loopDeal();
}
/* 线程处理 */
void* CCommendDeal::loopDeal (void)
{
    while (1)
    {
        while (!g_newQueue.empty())
        {
            // 获取新用户队列首元素
            CUser* pUser = g_newQueue.frontNew();
            // 删除新用户队列首元素
            g_newQueue.popNew();
            // 将新用户添加到映射
            m_userMap.insert (pair<int, CUser*> (pUser->getUserID(), pUser));
            // 将新用户向下扔给数据I/O模块
            g_addQueue.pushAdd (pUser);
        }

        if (g_cmdQueue.empty())
        {
            // 若没有指令需要处理，则沉睡200ms，再查看有无命令需要处理
            usleep (200000);
        }
        while (!g_cmdQueue.empty())
        {
            // 获取指令队列首元素
            HEADER* pHeader = g_cmdQueue.frontCmd();
            // 删除指令队列首元素
            g_cmdQueue.popCmd();

            int commID = pHeader->commID;  // 提取commID
            if (commID == logIn)  // 登录
			{
                logInDeal (pHeader);  // 登录指令处理
                clearRecv (pHeader);  // 整理接收缓存区
			}
            else if (commID == pChat)  // 私聊
			{
                pChatDeal (pHeader);  // 私聊指令处理
                clearRecv (pHeader);  // 整理接收缓存区
			}
            else if (commID == gChat)  // 群聊
			{
                gChatDeal (pHeader);  // 群聊指令处理
                clearRecv (pHeader);  // 整理接收缓存区
			}
            else if (commID == logOut) // 登出
			{
                logOutDeal (pHeader); // 登出指令处理
                clearRecv (pHeader);  // 整理接收缓存区
			}
            else if (commID == heartBeat) // 心跳
            {
                heartDeal (pHeader);  // 心跳指令处理
                clearRecv (pHeader);  // 整理接收缓存区
            }
            else
            {
                otherDeal (pHeader);  // 非法指令处理
                clearRecv (pHeader);  // 整理接收缓存区
            }

            delete pHeader;  // 销毁指令
        }  // end while

        // 用户心跳检查
        heartBeatCheck();
    }  // end while
}
/* 登录指令处理 */
void CCommendDeal::logInDeal (HEADER* pHeader)
{
    cout << "logIn" << endl;
    int userID = pHeader->userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户
    memcpy (it->second->m_name, it->second->m_recvBuf.m_recvBuf+16, 20); // 得到用户名
    memcpy (it->second->m_pwd, it->second->m_recvBuf.m_recvBuf+36, 20);  // 得到密码

    /* 用户名密码匹配 */

    // 匹配成功，返回logIn_SUCCESS
    HEADER header;
    header.version = VERSION;
    header.commID = logIn_SUCCESS;
    header.length = sizeof (header)+4;
    header.userID = userID;
    int to_userID = userID;

    memcpy (it->second->m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (it->second->m_sendBuf.m_sendBuf+sizeof (header), &to_userID, 4);
    it->second->m_sendBuf.m_allSize = header.length;  // 确定包的长度
}
/* 私聊指令处理 */
void CCommendDeal::pChatDeal (HEADER* pHeader)
{
    cout << "pChat" << endl;
    int length = pHeader->length;  // 提取包的长度
    int userID = pHeader->userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    int to_userID;
    memcpy (&to_userID, it->second->m_recvBuf.m_recvBuf+16, 4);   // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID); // 找到私聊对象

    if (that == m_userMap.end())  // 判断是否找到私聊对象
    {
        cout << "pChat_FAILURE" << endl;
        // 若未找到私聊对象，返回pChat_FAILURE
        HEADER header;
        header.version = VERSION;
        header.commID = pChat_FAILURE;
        header.length = sizeof (header)+4;
        header.userID = userID;
        int to_userID = userID;

        memcpy (it->second->m_sendBuf.m_sendBuf, &header, sizeof (header));
        memcpy (it->second->m_sendBuf.m_sendBuf+sizeof (header), &to_userID, 4);
        it->second->m_sendBuf.m_allSize = header.length;  // 确定包的长度
        return;
    }
    // 放到私聊对象发送缓冲区
    memcpy (that->second->m_sendBuf.m_sendBuf, it->second->m_recvBuf.m_recvBuf, 4);     // version
    memcpy (that->second->m_sendBuf.m_sendBuf+4, it->second->m_recvBuf.m_recvBuf+4, 4); // commID
    memcpy (that->second->m_sendBuf.m_sendBuf+8, &length, 4);        // length
    memcpy (that->second->m_sendBuf.m_sendBuf+12, &userID, 4);    // userID
    memcpy (that->second->m_sendBuf.m_sendBuf+16, &to_userID, 4); // to_userID
    memcpy (that->second->m_sendBuf.m_sendBuf+20, it->second->m_recvBuf.m_recvBuf+20, length-20); // message
    that->second->m_sendBuf.m_allSize = length;  // 确定包的长度
}
/* 群聊指令处理 */
void CCommendDeal::gChatDeal (HEADER* pHeader)
{
    cout << "gChat" << endl;
    int length = pHeader->length;  // 提取包的长度
    int userID = pHeader->userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    for (map<int, CUser*>::iterator that = m_userMap.begin(); that != m_userMap.end(); ++that)
	{
        memcpy (that->second->m_sendBuf.m_sendBuf, it->second->m_recvBuf.m_recvBuf, 4);     // version
        memcpy (that->second->m_sendBuf.m_sendBuf+4, it->second->m_recvBuf.m_recvBuf+4, 4); // commID
        memcpy (that->second->m_sendBuf.m_sendBuf+8, &length, 4);     // length
        memcpy (that->second->m_sendBuf.m_sendBuf+12, &userID, 4);    // userID
        int to_userID = that->second->getUserID();  // 得到to_userID
        memcpy (that->second->m_sendBuf.m_sendBuf+16, &to_userID, 4); // to_userID
        memcpy (that->second->m_sendBuf.m_sendBuf+20, it->second->m_recvBuf.m_recvBuf+20, length-20); // message
        that->second->m_sendBuf.m_allSize = length;  // 确定包的长度
	}
}
/* 登出指令处理 */
void CCommendDeal::logOutDeal (HEADER* pHeader)
{
    cout << "logOut" << endl;
    int userID = pHeader->userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户
    it->second->setDelete (1);  // 设置用户待删除，通知数据I/O模块
}
/* 心跳指令处理 */
void CCommendDeal::heartDeal (HEADER* pHeader)
{
    cout << "client heartBeat" << endl;
    int userID = pHeader->userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户
    it->second->m_lastRecvHeartBeat = time (NULL);  // 更新用户上次心跳时间
}
/* 非法指令处理 */
void CCommendDeal::otherDeal (HEADER* pHeader)
{
    cout << "unknown commID" << endl;
    int userID = pHeader->userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户
    it->second->m_suspectUser += 1;  // 可疑用户标志位+1
    if (it->second->m_suspectUser > SUSPECTUSER)
    {
        // 用户异常，强制清理
        logOutDeal (pHeader);
    }
}
/* 整理接收缓存区 */
void CCommendDeal::clearRecv (HEADER* pHeader)
{
    cout << "清理接收缓存区..." << endl;
    int userID = pHeader->userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户
    it->second->m_recvBuf.m_curSize = 0;
    it->second->m_recvBuf.m_allSize = 0;
    it->second->m_recvBuf.m_isHeaderFull = false;
    memset (it->second->m_recvBuf.m_recvBuf, 0, sizeof (it->second->m_recvBuf.m_recvBuf));
}
/* 用户心跳检查 */
void CCommendDeal::heartBeatCheck (void)
{
    time_t now = time (NULL);  // 获取系统当前时间
    for (map<int, CUser*>::iterator it = m_userMap.begin(); it != m_userMap.end(); ++it)
    {
        if (it->second->getDelete() == 2)  // 查看用户待删除标志位
        {
            map<int, CUser*>::iterator temp = it;
            m_userMap.erase (temp);  // 解除映射
            delete it->second;  // 销毁用户
        }

        if (now - it->second->m_lastRecvHeartBeat < 3*HEARTRATE)  // 检查用户心率
        {
            // 用户心跳正常
            continue;
        }
        else
        {
            // 用户心跳异常，设置用户待删除，通知数据I/O模块
            it->second->setDelete (1);
        }
    }  // end for
}
