/* 实现UICmd模块 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <iostream>
using namespace std;
#include "uicmd.h"
#include "user.h"
#include "sendqueue.h"
#include "packet.h"
/* 启动UICmd模块 */
void CUICmd::start (void) throw (ThreadException)
{
    cout << "启动UICmd模块开始..." << endl;
    int error = pthread_create (&m_tid, NULL, run, this);
    if (error)
        throw ThreadException (strerror (errno));
    cout << "启动UICmd模块完成！ " << endl;
}
/* 线程过程 */
void* CUICmd::run (void* arg)
{
    pthread_detach (pthread_self());
    return static_cast<CUICmd*> (arg)->loopUICmd();
}
/* 线程处理 */
void* CUICmd::loopUICmd (void)
{
	while (1)
	{
        cout << endl << "指令提示：0.logIn 1.pChat 2.gChat 3.transFile 4.logOut" << endl;

        int commID;
        cin >> commID;  // 输入指令

        switch (commID)
        {
        case logIn:
            logInCmd();  // 登录指令
            break;
        case pChat:
            pChatCmd();  // 私聊指令
            break;
        case gChat:
            gChatCmd();  // 群聊指令
            break;
        case transFile:
            transFileCmd();  // 传输文件指令
            break;
        case transFile_YES:
            yesFileCmd(); // 接受传输文件指令
			break;
        case transFile_NO:
            noFileCmd();  // 拒绝传输文件指令
            break;
        case logOut:
            logOutCmd();  // 登出指令
            break;
        default:
            cout << "指令错误！请重新输入！" << endl;
            break;
        }  // end switch

        usleep (500000);  // 500ms
    }  // end while

    return NULL;
}
/* 登录指令 */
void CUICmd::logInCmd (void)
{
    if (user.getUserID() > 0)
    {
        cout << "你已登录！请重新输入指令！" << endl;
        return;
    }
    HEADER header;
    header.version = VERSION;         // 版本
    header.commID = logIn;            // 登录指令
    header.userID = user.getUserID(); // 无效ID

    char userName[20] = { 0 }; // 用户名
    char userPWD[20] = { 0 };  // 密码
    cout << "请输入用户名和密码:" << endl;
    cin >> userName >> userPWD;

    header.length = sizeof (header)+sizeof (userName)+sizeof (userPWD)+sizeof (m_clientUDPAddr);  // 包的长度

    CSendBuf* pSendBuf = new CSendBuf;
    memcpy (pSendBuf->m_sendBuf, &header, sizeof (header));
    memcpy (pSendBuf->m_sendBuf+sizeof (header), userName, sizeof (userName));
    memcpy (pSendBuf->m_sendBuf+sizeof (header)+sizeof (userName), userPWD, sizeof (userPWD));
    memcpy (pSendBuf->m_sendBuf+sizeof (header)+sizeof (userName)+sizeof (userPWD), &m_clientUDPAddr, sizeof (m_clientUDPAddr));
    pSendBuf->m_allSize = header.length;  // 确定包的长度

    g_sendQueue.pushSend (pSendBuf);  // 压入待发送队列
}
/* 私聊指令 */
void CUICmd::pChatCmd (void)
{
    if (user.getUserID() < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    header.version = VERSION;         // 版本
    header.commID = pChat;            // 私聊指令
    header.userID = user.getUserID(); // 用户ID

    int to_userID;
    cout << "请输入私聊用户ID:" << endl;
    cin >> to_userID;
    char msg_buf[1024];
    cout << "请输入私聊消息:" << endl;
    cin >> msg_buf;

    header.length = sizeof (header)+sizeof (to_userID)+sizeof (msg_buf);  // 包的长度

    CSendBuf* pSendBuf = new CSendBuf;
    memcpy (pSendBuf->m_sendBuf, &header, sizeof (header));
    memcpy (pSendBuf->m_sendBuf+sizeof (header), &to_userID, sizeof (to_userID));
    memcpy (pSendBuf->m_sendBuf+sizeof (header)+sizeof (to_userID), msg_buf, sizeof (msg_buf));
    pSendBuf->m_allSize = header.length;  // 确定包的长度

    g_sendQueue.pushSend (pSendBuf);  // 压入待发送队列
}
/* 群聊指令 */
void CUICmd::gChatCmd (void)
{
    if (user.getUserID() < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    header.version = VERSION;         // 版本
    header.commID = gChat;            // 群聊指令
    header.userID = user.getUserID(); // 用户ID

    char msg_buf[1024];
    cout << "请输入群聊消息:" << endl;
    cin >> msg_buf;

    header.length = sizeof (header)+sizeof (msg_buf);  // 包的长度

    CSendBuf* pSendBuf = new CSendBuf;
    memcpy (pSendBuf->m_sendBuf, &header, sizeof (header));
    memcpy (pSendBuf->m_sendBuf+sizeof (header), msg_buf, sizeof (msg_buf));
    pSendBuf->m_allSize = header.length;  // 确定包的长度

    g_sendQueue.pushSend (pSendBuf);  // 压入待发送队列
}
/* 传输文件指令 */
void CUICmd::transFileCmd (void)
{
    if (user.getUserID() < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    header.version = VERSION;         // 版本
    header.commID = transFile;        // 传输文件指令
    header.userID = user.getUserID(); // 用户ID

    cout << "请输入目标用户ID:" << endl;
    cin >> user.m_fileRecvID;

    memset (user.m_filePath, 0, sizeof (user.m_filePath));
    cout << "请输入文件路径:" << endl;
    cin >> user.m_filePath;

	char fileName[20] = { 0 };
    char* tmp = basename (user.m_filePath);
    memcpy (fileName, tmp, sizeof (fileName));  // 得到文件名
	cout << "文件名 " << fileName << endl;

	struct stat st;
    if (stat (user.m_filePath, &st) < 0)
    {
        cout << "无此文件！" << endl;
        memset (user.m_filePath, 0, sizeof (user.m_filePath));
        user.m_fileSend = 0;
        user.m_fileRecvID = -1;
        return;
    }
    cout << "文件大小 " << st.st_size << endl;  // 得到文件大小

    int packets = st.st_size/1000;
    cout << "数据包总数 " << packets << endl;  // 得到数据包总数

    header.length = sizeof (header)+sizeof (m_fileRecvID)+sizeof (fileName)+sizeof (st.st_size)+sizeof (packets);  // 包的长度

    CSendBuf* pSendBuf = new CSendBuf;
    memcpy (pSendBuf->m_sendBuf, &header, sizeof (header));
    memcpy (pSendBuf->m_sendBuf+sizeof (header), &m_fileRecvID, sizeof (m_fileRecvID));
    memcpy (pSendBuf->m_sendBuf+sizeof (header)+sizeof (m_fileRecvID), fileName, sizeof (fileName));
    memcpy (pSendBuf->m_sendBuf+sizeof (header)+sizeof (m_fileRecvID)+sizeof (fileName), &st.st_size, sizeof (st.st_size));
    memcpy (pSendBuf->m_sendBuf+sizeof (header)+sizeof (m_fileRecvID)+sizeof (fileName)+sizeof (st.st_size), &packets, sizeof (packets));
    pSendBuf->m_allSize = header.length;  // 确定包的长度

    g_sendQueue.pushSend (pSendBuf);  // 压入待发送队列
}
/* 接受传输文件指令 */
void CUICmd::yesFileCmd (void)
{
    if (user.m_fileSize < 0)
	{
        cout << "指令错误！请重新输入！" << endl;
		return;
    }
    HEADER header;
    header.version = VERSION;         // 版本
    header.commID = transFile_YES;    // 接受传输文件指令
    header.length = sizeof (header)+sizeof (user.m_fileSendID);  // 包的长度
    header.userID = user.getUserID(); // 用户ID

    CSendBuf* pSendBuf = new CSendBuf;
    memcpy (pSendBuf->m_sendBuf, &header, sizeof (header));
    memcpy (pSendBuf->m_sendBuf+sizeof (header), &user.m_fileSendID, sizeof (user.m_fileSendID));
    pSendBuf->m_allSize = header.length;  // 确定包的长度

    g_sendQueue.pushSend (pSendBuf);  // 压入待发送队列
}
/* 拒绝传输文件指令 */
void CUICmd::noFilelCmd (void)
{
    if (user.m_fileSize < 0)
	{
        cout << "指令错误！请重新输入！" << endl;
		return;
    }
    HEADER header;
    header.version = VERSION;         // 版本
    header.commID = transFile_NO;     // 拒绝传输文件指令
    header.length = sizeof (header)+sizeof (user.m_fileSendID);  // 包的长度
    header.userID = user.getUserID(); // 用户ID

    CSendBuf* pSendBuf = new CSendBuf;
    memcpy (pSendBuf->m_sendBuf, &header, sizeof (header));
    memcpy (pSendBuf->m_sendBuf+sizeof (header), &user.m_fileSendID, sizeof (user.m_fileSendID));
    pSendBuf->m_allSize = header.length;  // 确定包的长度

    g_sendQueue.pushSend (pSendBuf);  // 压入待发送队列

    memset (user.m_fileName, 0, sizeof (user.m_fileName));
    user.m_fileSize = -1;
    user.m_fileSendID = -1;
}
/* 登出指令 */
void CUICmd::logOutCmd (void)
{
    if (user.getUserID() < 0)
    {
        cout << "你还没登录！" << endl;
        return;
    }
    HEADER header;
    header.version = VERSION;         // 版本
    header.commID = logOut;           // 登出指令
    header.length = sizeof (header);  // 包的长度
    header.userID = user.getUserID(); // 用户ID

    CSendBuf* pSendBuf = new CSendBuf;
    memcpy (pSendBuf->m_sendBuf, &header, sizeof (header));
    pSendBuf->m_allSize = header.length;  // 确定包的长度

    g_sendQueue.pushSend (pSendBuf);  // 压入待发送队列

    cout << "你已登出！" << endl;
}

