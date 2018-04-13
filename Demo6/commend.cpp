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
#include "queue.h"
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
            HEADER* pHeader = g_cmdQueue.frontCmd();
            // 删除指令队列首元素
            g_cmdQueue.popCmd();

            int commID = pHeader->commID;  // 提取commID
            if (commID == logIn)  // 登录
            {
                logInDeal (pHeader);  // 登录指令处理
			}
            else if (commID == pChat)  // 私聊
			{
                pChatDeal (pHeader);  // 私聊指令处理
			}
            else if (commID == gChat)  // 群聊
			{
                gChatDeal (pHeader);  // 群聊指令处理
			}
            else if (commID == transFile)  // 传输文件
			{
                transFileDeal (pHeader);  // 传输文件指令处理
			}
            else if (commID == transFile_ING)  // 传输文件中
            {
                transFileIngDeal (pHeader);  // 传输文件中指令处理
            }
            else if (commID == transFile_OK) // 接收文件完成
            {
                transFileOKDeal (pHeader);  // 接收文件完成指令处理
            }
            else if (commID == transFile_YES  || commID == transFile_NO)
			{
                yesnoDeal (pHeader);   // yes/no指令处理
			}
            else if (commID == logOut)  // 登出
			{
                logOutDeal (pHeader);  // 登出指令处理
			}
            else if (commID == heartBeat)  // 心跳
            {
                heartDeal (pHeader);  // 心跳指令处理
            }
            else
            {
                otherDeal (pHeader);  // 非法指令处理
            }
        }  // end while

        // 用户心跳检查
        heartBeatCheck();
    }  // end while

    return NULL;
}
/* 登录指令处理 */
void CCommendDeal::logInDeal (HEADER* pHeader)
{
    cout << "logIn" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID); // 找到用户
    memcpy (it->second->m_name, pRecvBuf->m_recvBuf, sizeof (it->second->m_name));    // 得到用户名
    memcpy (it->second->m_pwd, pRecvBuf->m_recvBuf+sizeof (it->second->m_name), sizeof (it->second->m_pwd));  // 得到密码
    memcpy (&it->second->m_userUDPAddr, pRecvBuf->m_recvBuf+sizeof (it->second->m_name)+sizeof (it->second->m_pwd), sizeof (it->second->m_userUDPAddr));  // 得到用户UDP协议地址

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

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* 私聊指令处理 */
void CCommendDeal::pChatDeal (HEADER* pHeader)
{
    cout << "pChat" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
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

        delete pRecvBuf;  // 销毁发送缓冲区
        return;
    }

    cout << "拷贝到私聊用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), pRecvBuf->m_recvBuf, length-sizeof (pRecvBuf->header));
    that->second->m_sendBuf.m_allSize = length;  // 确定包的长度

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* 群聊指令处理 */
void CCommendDeal::gChatDeal (HEADER* pHeader)
{
    cout << "gChat" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
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

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* 传输文件指令处理 */
void CCommendDeal::transFileDeal (HEADER* pHeader)
{
	cout << "transFile" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    int length = pRecvBuf->header.length;  // 提取包的长度
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    int to_userID;
    memcpy (&to_userID, pRecvBuf->m_recvBuf, sizeof (to_userID));  // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID);  // 找到目标用户

    if (that == m_userMap.end())  // 判断是否找到目标用户
    {
        transFailDeal (it->second, transFile_SENDFAIL);  // 传输文件失败处理

        delete pRecvBuf;  // 销毁发送缓冲区
		return;
    }

    cout << "拷贝到目标用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), pRecvBuf->m_recvBuf, length-sizeof (pRecvBuf->header));
    that->second->m_sendBuf.m_allSize = length;  // 确定包的长度

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* 传输文件中指令处理 */
void CCommendDeal::transFileIngDeal (HEADER* pHeader)
{
    cout << "transFile_ING" << endl;
    PACKET* pPacket = (PACKET*)pHeader;
    map<int, CUser*>::iterator that = m_userMap.find (pPacket->to_userID);  // 找到接收方

    if (that == m_userMap.end())  // 判断是否找到目标用户
    {
        map<int, CUser*>::iterator it = m_userMap.find (pPacket->header.userID);  // 找到发送方
        transFailDeal (it->second, transFile_SENDFAIL);

        delete pPacket;
        return;
    }

    g_queue.push ((void*)pPacket);  // 通过队列传递PACKET
    g_queue.push ((void*)(&that->second->m_userUDPAddr));  // 通过队列传递接收方UDP协议地址
}
/* 接收文件完成指令处理 */
void CCommendDeal::transFileOKDeal (HEADER* pHeader)
{
    cout << "transFile_OK" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    int length = pRecvBuf->header.length;  // 提取包的长度

    int to_userID;
    memcpy (&to_userID, pRecvBuf->m_recvBuf, sizeof (to_userID));  // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID);

    cout << "拷贝到传输文件用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), &to_userID, sizeof (to_userID));
    that->second->m_sendBuf.m_allSize = length;  // 确定包的长度

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* yes/no指令处理 */
void CCommendDeal::yesnoDeal (HEADER* pHeader)
{
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    if (pRecvBuf->header.commID == transFile_YES)
        cout << "transFile_YES" << endl;
    else
        cout << "transFile_NO" << endl;

    int length = pRecvBuf->header.length;  // 提取包的长度
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    int to_userID;
    memcpy (&to_userID, pRecvBuf->m_recvBuf, sizeof (to_userID));  // 提取to_userID
    map<int, CUser*>::iterator that = m_userMap.find (to_userID);

    if (that == m_userMap.end())
	{
        transFailDeal (it->second, transFile_RECVFAIL);  // 传输文件失败处理

        delete pRecvBuf;  // 销毁发送缓冲区
		return;
	}

    cout << "拷贝到传输文件用户发送缓冲区..." << endl;
    memcpy (that->second->m_sendBuf.m_sendBuf, &pRecvBuf->header, sizeof (pRecvBuf->header));
    memcpy (that->second->m_sendBuf.m_sendBuf+sizeof (pRecvBuf->header), &to_userID, sizeof (to_userID));
	that->second->m_sendBuf.m_allSize = length;  // 确定包的长度

    delete pRecvBuf;  // 销毁发送缓冲区
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
void CCommendDeal::logOutDeal (HEADER* pHeader)
{
    cout << "logOut" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    cout << "设置用户待删除，通知数据I/O模块！" << endl;
    it->second->setDelete (1);  // 设置用户待删除，通知数据I/O模块

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* 心跳指令处理 */
void CCommendDeal::heartDeal (HEADER* pHeader)
{
    cout << "client heartBeat" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    it->second->m_lastRecvHeartBeat = time (NULL);  // 更新用户上次心跳时间

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* 非法指令处理 */
void CCommendDeal::otherDeal (HEADER* pHeader)
{
    cout << "unknown commID" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    int userID = pRecvBuf->header.userID;  // 提取用户ID
    map<int, CUser*>::iterator it = m_userMap.find (userID);  // 找到用户

    it->second->m_suspectUser += 1;  // 可疑用户标志位+1
    if (it->second->m_suspectUser > SUSPECTUSER)
    {
        // 用户异常，强制清理
        logOutDeal (pHeader);
    }

    delete pRecvBuf;  // 销毁发送缓冲区
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
