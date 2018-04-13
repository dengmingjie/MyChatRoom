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
            // 若没有指令需要处理，则沉睡100us，再查看有无命令需要处理
            usleep (100);
        }
        while (!g_cmdQueue.empty())
        {
            // 获取指令队列首元素
            CRecvBuf* pRecvBuf = (CRecvBuf*)g_cmdQueue.frontCmd();
            // 删除指令队列首元素
            g_cmdQueue.popCmd();

            int commID = pRecvBuf->header.commID;  // 提取commID
            if (commID == logIn)  // 登录
			{
                logInDeal (pRecvBuf);  // 登录指令处理
			}
            else if (commID == pChat) // 私聊
			{
                pChatDeal (pRecvBuf);  // 私聊指令处理
			}
            else if (commID == gChat) // 群聊
			{
                gChatDeal (pRecvBuf);  // 群聊指令处理
			}
            else if (commID == transFile) // 传输文件
			{
                transFileDeal (pRecvBuf);  // 传输文件指令处理
			}
			else if (commID == transFile_YES || commID == transFile_NO)
			{
                yesnoDeal (pRecvBuf);  // yes/no指令处理
			}
			else if (commID == transFile_ING)  // 传输文件中
			{
                transIngDeal (pRecvBuf); // 传输文件中指令处理
			}
            else if (commID == logOut)  // 登出
			{
                logOutDeal (pRecvBuf);   // 登出指令处理
			}
            else if (commID == heartBeat) // 心跳
            {
                heartDeal (pRecvBuf);  // 心跳指令处理
            }
            else
            {
                otherDeal (pRecvBuf);  // 非法指令处理
            }

            delete pRecvBuf;  // 销毁发送缓冲区
        }  // end while

        // 用户心跳检查
        heartBeatCheck();
    }  // end while
}
/* 登录指令处理 */
void CCommendDeal::logInDeal (CRecvBuf* pRecvBuf)
{
    cout << "logIn" << endl;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID); // 找到用户
    memcpy (it->second->m_name, pRecvBuf->m_recvBuf, sizeof (it->second->m_name));    // 得到用户名
    memcpy (it->second->m_pwd, pRecvBuf->m_recvBuf+sizeof (it->second->m_name), sizeof (it->second->m_pwd));  // 得到密码

    /* 用户名/密码匹配 */

    // 匹配成功，返回logIn_SUCCESS
    HEADER header;
    header.version = VERSION;
    header.commID = logIn_SUCCESS;
    header.length = sizeof (header)+sizeof (userID);
    header.userID = userID;
    int to_userID = userID;

    cout << "拷贝到登录用户发送缓冲区..." << endl;
    memcpy (it->second->m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (it->second->m_sendBuf.m_sendBuf+sizeof (header), &to_userID, sizeof (to_userID));
    it->second->m_sendBuf.m_allSize = header.length;  // 确定包的长度
}
/* 私聊指令处理 */
void CCommendDeal::pChatDeal (CRecvBuf* pRecvBuf)
{
    cout << "pChat" << endl;
    int length = pRecvBuf->header.length;  // 提取包的长度
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    int to_userID;
    memcpy (&to_userID, pRecvBuf->m_recvBuf, sizeof (to_userID));  // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID);  // 找到私聊用户

    if (that == m_userMap.end())  // 判断是否找到私聊用户
    {
        // 若未找到私聊用户，返回pChat_FAILURE
        cout << "pChat_FAILURE" << endl;
        HEADER header;
        header.version = VERSION;
        header.commID = pChat_FAILURE;
        header.length = sizeof (header)+sizeof (userID);
        header.userID = userID;
        int to_userID = userID;

        cout << "拷贝到私聊失败用户发送缓冲区..." << endl;
        memcpy (it->second->m_sendBuf.m_sendBuf, &header, sizeof (header));
        memcpy (it->second->m_sendBuf.m_sendBuf+sizeof (header), &to_userID, sizeof (to_userID));
        it->second->m_sendBuf.m_allSize = header.length;  // 确定包的长度
        return;
    }

    cout << "拷贝到私聊用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), pRecvBuf->m_recvBuf, length-sizeof (pRecvBuf->header));
    that->second->m_sendBuf.m_allSize = length;  // 确定包的长度
}
/* 群聊指令处理 */
void CCommendDeal::gChatDeal (CRecvBuf* pRecvBuf)
{
    cout << "gChat" << endl;
    int length = pRecvBuf->header.length;  // 提取包的长度
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    cout << "拷贝到所有在线用户发送缓冲区..." << endl;
    for (map<int, CUser*>::iterator that = m_userMap.begin(); that != m_userMap.end(); ++that)
	{
        memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
        int to_userID = that->second->getUserID();  // 得到to_userID
        memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), &to_userID, sizeof (to_userID));
        memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header)+sizeof (to_userID), pRecvBuf->m_recvBuf, length-sizeof (pRecvBuf->header));
        that->second->m_sendBuf.m_allSize = length; // 确定包的长度
	}
}
/* 传输文件指令处理 */
void CCommendDeal::transFileDeal (CRecvBuf* pRecvBuf)
{
	cout << "transFile" << endl;
    int length = pRecvBuf->header.length;  // 提取包的长度
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    int to_userID;
    memcpy (&to_userID, pRecvBuf->m_recvBuf, sizeof (to_userID));  // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID);  // 找到目标用户

    if (that == m_userMap.end())  // 判断是否找到目标用户
    {
        transFailDeal (it->second, transFile_SENDFAIL);  // 传输文件失败处理
		return;
    }

    cout << "拷贝到目标用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), pRecvBuf->m_recvBuf, length-sizeof (pRecvBuf->header));
    that->second->m_sendBuf.m_allSize = length;  // 确定包的长度
}
/* yes/no指令处理 */
void CCommendDeal::yesnoDeal (CRecvBuf* pRecvBuf)
{
	cout << "transFile YES/NO" << endl;
    int length = pRecvBuf->header.length;  // 提取包的长度
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    int to_userID;
    memcpy (&to_userID, pRecvBuf->m_recvBuf, sizeof (to_userID));  // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID);  // 找到目标用户

    if (that == m_userMap.end())  // 判断是否找到目标用户
	{
        transFailDeal (it->second, transFile_RECVFAIL);  // 传输文件失败处理
		return;
	}

    cout << "拷贝到传输文件用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), &to_userID, sizeof (to_userID));
	that->second->m_sendBuf.m_allSize = length;  // 确定包的长度
}
/* 传输文件中指令处理 */
int n;
void CCommendDeal::transIngDeal (CRecvBuf* pRecvBuf)
{
	cout << "transFile_ING" << endl;
    int length = pRecvBuf->header.length;  // 提取包的长度
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    int to_userID;
    memcpy (&to_userID, pRecvBuf->m_recvBuf, sizeof (to_userID));  // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID);  // 找到目标用户

#if 0
    cout << "写入文件完成！" << endl;
    FILE* fp = fopen ("test.mkv", "ab");
    fwrite (pRecvBuf->m_recvBuf+4, length-20, 1, fp);
    fclose (fp);
    n += length-20;
    cout << "write " << n << endl;
    cout << "写入文件完成！" << endl;
#endif

    if (that == m_userMap.end())  // 判断是否找到目标用户
	{
        transFailDeal (it->second, transFile_SENDFAIL);  // 传输文件失败处理
		return;
	}

waitSendBufEmpty:
    if (!that->second->m_sendBufEmpty)
    {
        goto waitSendBufEmpty;
    }

    cout << "拷贝到目标用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), pRecvBuf->m_recvBuf, length-sizeof (pRecvBuf->header));
	that->second->m_sendBuf.m_allSize = length;  // 确定包的长度

    that->second->m_sendBufEmpty = false;

    it->second->m_lastRecvHeartBeat = time (NULL);  // 更新用户上次心跳时间
}
/* 传输文件失败处理 */
void CCommendDeal::transFailDeal (CUser* pUser, comm cmd)
{
    HEADER header;
    header.version = VERSION;
    header.commID = cmd;
    header.length = sizeof (header)+sizeof (pUser->getUserID());
    header.userID = pUser->getUserID();
    int to_userID = pUser->getUserID();

    memcpy (pUser->m_sendBuf.m_sendBuf, &header, sizeof (header));
    memcpy (pUser->m_sendBuf.m_sendBuf+sizeof (header), &to_userID, sizeof (to_userID));
    pUser->m_sendBuf.m_allSize = header.length;  // 确定包的长度
}
/* 登出指令处理 */
void CCommendDeal::logOutDeal (CRecvBuf* pRecvBuf)
{
    cout << "logOut" << endl;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    cout << "设置用户待删除，通知数据I/O模块！" << endl;
    it->second->setDelete (1);  // 设置用户待删除，通知数据I/O模块
}
/* 心跳指令处理 */
void CCommendDeal::heartDeal (CRecvBuf* pRecvBuf)
{
    cout << "client heartBeat" << endl;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    it->second->m_lastRecvHeartBeat = time (NULL);  // 更新用户上次心跳时间
}
/* 非法指令处理 */
void CCommendDeal::otherDeal (CRecvBuf* pRecvBuf)
{
    cout << "unknown commID" << endl;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    it->second->m_suspectUser += 1;  // 可疑用户标志位+1
    if (it->second->m_suspectUser > SUSPECTUSER)
    {
        // 用户异常，强制清理
        logOutDeal (pRecvBuf);
    }
}
/* 用户心跳检查 */
void CCommendDeal::heartBeatCheck (void)
{
    time_t now = time (NULL);  // 获取系统当前时间
    for (map<int, CUser*>::iterator it = m_userMap.begin(); it != m_userMap.end(); ++it)
    {
        if (it->second->getDelete() == 2)  // 查看用户待删除标志位
        {
            cout << "发现用户可删除，销毁用户！" << endl;
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
            cout << "用户心跳异常，设置用户待删除，通知数据I/O模块！" << endl;
            it->second->setDelete (1);
        }
    }  // end for
}
