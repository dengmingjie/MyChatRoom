/* 指令处理模块 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <map>
using namespace std;
#include "commend.h"
#include "userqueue.h"
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
        if (g_userQueue.empty())
        {
            sleep (2);
        }
        while (!g_userQueue.empty())  // 判断队列是否为空
        {
            int commID;
            memcpy(&commID, g_userQueue.frontUser()->m_recvBuf.m_recvBuf+4, 4);  // 提取commID
            if (commID == logIn)  // 登录
			{
				logInDeal (g_userQueue.frontUser());  // 登录命令处理
				clearRecv (g_userQueue.frontUser());  // 整理接收缓存区
			}
            else if (commID == pChat)  // 私聊
			{
				pChatDeal (g_userQueue.frontUser());  // 私聊命令处理
				clearRecv (g_userQueue.frontUser());  // 整理接收缓存区
			}
            else if (commID == gChat)  // 群聊
			{
				gChatDeal (g_userQueue.frontUser());  // 群聊命令处理
				clearRecv (g_userQueue.frontUser());  // 整理接收缓存区
			}
            else if (commID == logOut) // 登出
			{
				logOutDeal (g_userQueue.frontUser()); // 登出命令处理
			}
            else if (commID == heartBeat) // 心跳
            {
                cout << "client heartBeat" << endl;
                g_userQueue.frontUser()->m_lastRecvHeartBeat = time (NULL);  // 更新用户上次心跳时间
                clearRecv (g_userQueue.frontUser());  // 整理接收缓存区
            }
            else  // 非法命令
            {
                g_userQueue.frontUser()->m_suspectUser += 1;  // 可疑用户标志位+1
                if (g_userQueue.frontUser()->m_suspectUser > SUSPECTUSER)
                {
                    // 用户异常，强制清理
                    logOutDeal (g_userQueue.frontUser());
                }
            }
            g_userQueue.popUser();  // 删除队列首元素
        }  // end while

        // 用户心跳检查
        heartBeatCheck();
    }  // end while
}
/* 登录命令处理 */
void CCommendDeal::logInDeal (CUser* pUser)
{
    cout << "logIn" << endl;
    memcpy (&pUser->m_name, pUser->m_recvBuf.m_recvBuf+20, 20); // 得到用户名
    memcpy (&pUser->m_pwd, pUser->m_recvBuf.m_recvBuf+40, 20);  // 得到密码

    // 登录成功，返回logIn_SUCCESS
    memcpy (pUser->m_sendBuf.m_sendBuf, pUser->m_recvBuf.m_recvBuf, 4);  // version
    comm logInSUCCESS = logIn_SUCCESS;
    memcpy (pUser->m_sendBuf.m_sendBuf+4, &logInSUCCESS, 4);  // commID
    int length = 20;
    memcpy (pUser->m_sendBuf.m_sendBuf+8, &length, 4);  // length
    memcpy (pUser->m_sendBuf.m_sendBuf+12, &(pUser->m_userID), 4);  // userID
    memcpy (pUser->m_sendBuf.m_sendBuf+16, &(pUser->m_userID), 4);  // to_userID
    memcpy (&pUser->m_sendBuf.m_allSize, &length, 4);
}
/* 私聊命令处理 */
void CCommendDeal::pChatDeal (CUser* pUser)
{
    cout << "pChat" << endl;
    int to_userID;
    memcpy(&to_userID, pUser->m_recvBuf.m_recvBuf+16, 4);  // 提取to_userID
    map<int, CUser*>::iterator it = g_userMap.find (to_userID);  // 找到私聊对象
    if (it == g_userMap.end())
    {
        cout << "pChat_FAILURE" << endl;
        // 若未找到私聊用户，返回pChat_FAILURE
        memcpy (pUser->m_sendBuf.m_sendBuf, pUser->m_recvBuf.m_recvBuf, 4);  // version
        comm pChatFAILURE = pChat_FAILURE;
        memcpy (pUser->m_sendBuf.m_sendBuf+4, &pChatFAILURE, 4);  // commID
        int length = 20;
        memcpy (pUser->m_sendBuf.m_sendBuf+8, &length, 4);  // length
        memcpy (pUser->m_sendBuf.m_sendBuf+12, &(pUser->m_userID), 4);  // userID
        memcpy (pUser->m_sendBuf.m_sendBuf+16, &(pUser->m_userID), 4);  // to_userID
        memcpy (&pUser->m_sendBuf.m_allSize, &length, 4);
        return;
    }
    memcpy (it->second->m_sendBuf.m_sendBuf, pUser->m_recvBuf.m_recvBuf, 4);  // version
    memcpy (it->second->m_sendBuf.m_sendBuf+4, pUser->m_recvBuf.m_recvBuf+4, 4);  // commID
    int length;
    memcpy (&length, pUser->m_recvBuf.m_recvBuf+8, 4);  // 提取length
    memcpy (it->second->m_sendBuf.m_sendBuf+8, &length, 4);  // length
    memcpy (it->second->m_sendBuf.m_sendBuf+12, &(pUser->m_userID), 4);  // userID
    memcpy (it->second->m_sendBuf.m_sendBuf+16, &to_userID, 4);  // to_userID
    memcpy (it->second->m_sendBuf.m_sendBuf+20, pUser->m_recvBuf.m_recvBuf+20, length-20);  // message
    memcpy (&it->second->m_sendBuf.m_allSize, &length, 4);
}
/* 群聊命令处理 */
void CCommendDeal::gChatDeal (CUser* pUser)
{
    cout << "gChat" << endl;
    for (map<int, CUser*>::iterator it = g_userMap.begin(); it != g_userMap.end(); ++it)
	{
        memcpy (it->second->m_sendBuf.m_sendBuf, pUser->m_recvBuf.m_recvBuf, 4);  // version
        memcpy (it->second->m_sendBuf.m_sendBuf+4, pUser->m_recvBuf.m_recvBuf+4, 4);  // commID
        int length;
        memcpy (&length, pUser->m_recvBuf.m_recvBuf+8, 4);  // 提取length
        memcpy (it->second->m_sendBuf.m_sendBuf+8, &length, 4);  // length
        memcpy (it->second->m_sendBuf.m_sendBuf+12, &(pUser->m_userID), 4);  // userID
        memcpy (it->second->m_sendBuf.m_sendBuf+16, &(it->second->m_userID), 4);  // to_userID
        memcpy (it->second->m_sendBuf.m_sendBuf+20, pUser->m_recvBuf.m_recvBuf+20, length-20);  // message
        memcpy (&it->second->m_sendBuf.m_allSize, &length, 4);
	}
}
/* 登出命令处理 */
void CCommendDeal::logOutDeal (CUser* pUser)
{
    delUser (pUser);
}
/* 整理接收缓存区 */
void CCommendDeal::clearRecv (CUser* pUser)
{
    cout << "清理接收缓存区..." << endl;
    pUser->m_recvBuf.m_curSize = 0;
    pUser->m_recvBuf.m_allSize = 0;
    pUser->m_recvBuf.m_isHeaderFull = false;
    memset (pUser->m_recvBuf.m_recvBuf, 0, sizeof (pUser->m_recvBuf.m_recvBuf));
}
/* 用户心跳检查 */
void CCommendDeal::heartBeatCheck (void)
{
    time_t now = time (NULL);  // 获取系统当前时间
    for (map<int, CUser*>::iterator it = g_userMap.begin(); it != g_userMap.end(); ++it)
    {
        if (now - it->second->m_lastRecvHeartBeat < 3*HEARTRATE)  // 检查用户心率
        {
            continue;  // 用户心跳正常
        }
        else
        {
            // 用户异常，清理用户
            logOutDeal (it->second);
        }
    }  // end for
}
