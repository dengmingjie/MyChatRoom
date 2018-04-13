/* 发送文件模块 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;
#include "sendfile.h"
#include "user.h"
#include "errnos.h"
#include "packet.h"
/* 启动传输文件模块 */
void CSendFile::start (void) throw (ThreadException)
{
    cout << "启动发送文件模块开始..." << endl;
    int error = pthread_create (&m_tid, NULL, run, this);
    if (error)
        throw ThreadException (strerror (errno));
    cout << "启动发送文件模块完成！ " << endl;
}
/* 线程过程 */
void* CSendFile::run (void* arg)
{
    pthread_detach (pthread_self());
    return static_cast<CSendFile*> (arg)->loopSend();
}
/* 线程处理 */
void* CSendFile::loopSend (void)
{
    while (1)
    {
        if (user.m_fileSendYes)
        {
            PACKET packet;
            memset (&packet, 0, sizeof (packet));
            int ret = fread (packet.data, 1, sizeof (packet.data), user.m_fp);  // 读取数据到buf

            packet.header.version = VERSION;
            packet.header.commID = transFile_ING;
            packet.header.length = sizeof (packet);
            packet.header.userID = user.getUserID();
            packet.to_userID = user.m_fileRecvID;
            packet.packetNum = ++(user.m_packetNum);

            user.m_fileSend += ret;  // 保存当前发送长度
            cout << "send " << user.m_fileSend << endl;

            sockaddr_in len = sizeof (user.m_serverUDPAddr);
            ret = sendto (user.m_UDPSocket, packet, sizeof (packet), 0, (struct sockaddr*)&user.m_serverUDPAddr, &len);
            if (ret < 0)
            {
                cout << "sendto() ";
                errnos();
                continue;
            }
        }
        else
        {
            // 发送UDP心跳包
        }
    }  // end while

	return NULL;
}
