/* 指令处理模块 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
using namespace std;
#include "commend.h"
#include "user.h"
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

            cout << "指令处理开始..." << endl;
            int commID = pHeader->commID;  // 提取commID
			if (commID == logIn_SUCCESS)  // 登录成功
			{
                logInSuccessDeal (pHeader);
			}
			else if (commID == pChat)  // 私聊
			{
                pChatDeal (pHeader);
			}
			else if (commID == gChat)  // 群聊
			{
                gChatDeal (pHeader);
			}
			else if (commID == pChat_FAILURE)  // 私聊失败
			{
                pChatFailureDeal (pHeader);
			}
			else if (commID == transFile)  // 传输文件
			{
                transFileDeal (pHeader);
			}
            else if (commID == transFile_ING)  // 传输文件中
            {
                transFileIngDeal (pHeader);
            }
            else if (commID == transFile_SENDFAIL)  // 发送文件失败
			{
                SendFileDeal (pHeader);
			}
			else if (commID == transFile_RECVFAIL)  // 接收文件失败
			{
                RecvFileDeal (pHeader);
			}
			else if (commID == transFile_YES)  // 接受传输文件
			{
                yesDeal (pHeader);
			}
			else if (commID == transFile_NO)  // 拒绝传输文件
			{
                noDeal (pHeader);
			}
            else if (commID == transFile_OK)  // 传输文件完成
            {
                transFileOKDeal (pHeader);
            }
            else if (commID == heartBeat)  // 心跳
			{
                heartDeal (pHeader);
			}
			cout << "指令处理完成！" << endl;
        }  // end while
    }  // end while

    return NULL;
}
/* 登录成功指令处理 */
void CCommendDeal::logInSuccessDeal (HEADER* pHeader)
{
    cout << "logIn_SUCCESS" << endl;
    CRecvBuf* pRecvBuf = (CRecvBuf*)pHeader;
    m_userID = pRecvBuf->header.userID;  // 提取用户ID
    cout << "my userID: " << m_userID << endl;  // 提示用户ID

    delete pRecvBuf;  // 销毁发送缓冲区
}
/* 私聊指令处理 */
void CCommendDeal::pChatDeal (HEADER* pHeader)
{
    cout << "pChat" << endl;
    int userID;
    memcpy (&userID, m_recvBuf.m_recvBuf+12, 4);  // 提取userID
    cout << userID << " to me:" << endl;

    char msg[1024+1] = { 0 };
    memcpy (msg, m_recvBuf.m_recvBuf+20, m_recvBuf.m_allSize-20);  // 提取消息
    cout << msg << endl;  // 提示消息
}
/* 群聊指令处理 */
void CCommendDeal::gChatDeal (HEADER* pHeader)
{
    cout << "gChat" << endl;
    int userID;
    memcpy (&userID, m_recvBuf.m_recvBuf+12, 4);  // 提取userID
    cout << userID << " to all:" << endl;

    char msg[1024+1] = { 0 };
    memcpy (msg, m_recvBuf.m_recvBuf+20, m_recvBuf.m_allSize-20);  // 提取消息
    cout << msg << endl;  // 提示消息
}
/* 私聊失败指令处理 */
void CCommendDeal::pChatFailureDeal (HEADER* pHeader)
{
    cout << "pChat_FAILURE" << endl;
    cout << "私聊失败，用户不存在或已下线！" << endl;
}
/* 传输文件指令处理 */
void CCommendDeal::transFileDeal (HEADER* pHeader)
{
    cout <<  "transFile" << endl;
    memcpy (&m_fileSendID, m_recvBuf.m_recvBuf+12, sizeof (m_fileSendID)); // 提取userID
    memcpy (m_fileName, m_recvBuf.m_recvBuf+20, sizeof (m_fileName));      // 提取文件名
    memcpy (&m_fileSize, m_recvBuf.m_recvBuf+40, sizeof (m_fileSize));     // 提取文件大小
    cout << m_fileSendID << " to me: " << m_fileName << ' ' << m_fileSize << endl;
    cout << "接受还是拒绝？(y/n)(10/11)" << endl;
}
/* 传输文件中指令处理 */
void CCommendDeal::transFileIngDeal (HEADER* pHeader)
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
/* 发送文件失败指令处理 */
void CCommendDeal::SendFileDeal (HEADER* pHeader)
{
    cout << "transFile_SENDFAIL" << endl;
    cout << "传输文件失败，用户不存在或已下线！" << endl;
    memset (m_filePath, 0, sizeof (m_filePath));
    m_fileSend = 0;
    m_fileRecvID = -1;
    m_fileSendYes = false;
}
/* 接收文件失败指令处理 */
void CCommendDeal::RecvFileDeal (HEADER* pHeader)
{
    cout << "transFile_RECVFAIL" << endl;
    cout << "接收文件失败，用户不存在或已下线！" << endl;
    memset (m_fileName, 0, sizeof (m_fileName));
    m_fileSize = -1;
    m_fileSendID = -1;
}
/* yes指令处理 */
void CCommendDeal::yesDeal (HEADER* pHeader)
{
    cout << "transFile_YES" << endl;
    cout << "目标用户接受传输文件！" << endl;
    m_fileSendYes = true;
    m_fp = fopen (m_filePath, "rb");  // 只读方式打开二进制文件
}
/* no指令处理 */
void CCommendDeal::noDeal (HEADER* pHeader)
{
    cout << "transFile_NO" << endl;
    cout << "目标用户拒绝传输文件！" << endl;
    memset (m_filePath, 0, sizeof (m_filePath));
    m_fileSend = 0;
    m_fileRecvID = -1;
    m_fileSendYes = false;
}
/* 接收文件完成指令处理 */
void CCommendDeal::transFileOKDeal (HEADER* pHeader)
{

}
/* 心跳指令处理 */
void CCommendDeal::heartDeal (HEADER* pHeader)
{
    cout << "server heartBeat" << endl;
    m_lastRecvHeartBeat = time (NULL);  // 更新服务器上次心跳时间
}
/* 服务器心跳检查 */
void CCommendDeal::heartBeatCheck (void) throw (ClientException)
{
    time_t now = time (NULL);  // 获取系统当前时间
    if (now - user.m_lastRecvHeartBeat < 3*HEARTRATE)  // 检查服务器心率
    {
        return;  // 服务器心跳正常
    }
    else
    {
        // 服务器心跳异常，做相关操作，比如重连等，return
        cout << "服务器心跳异常！" << endl;
    }
}
