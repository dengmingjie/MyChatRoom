/* recvfrom模块 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <iostream>
using namespace std;
#include "recvfrom.h"
#include "cmdqueue.h"
#include "errnos.h"
#include "packet.h"
/* 构造器 */
CRecvfrom::CRecvfrom (int UDPfd)
{
    m_UDPfd = UDPfd;
}
/* 析构器 */
CRecvfrom::~CRecvfrom (void) {}
/* 启动recvfrom模块 */
void CRecvfrom::start (void) throw (ThreadException)
{
    cout << "启动recvfrom模块开始..." << endl;
    int error = pthread_create (&m_tid, NULL, run, this);
    if (error)
        throw ThreadException (strerror (errno));
    cout << "启动recvfrom模块完成！ " << endl;
}
/* 线程过程 */
void* CRecvfrom::run (void* arg)
{
    pthread_detach (pthread_self());
    return static_cast<CRecvfrom*> (arg)->loopRecvfrom();
}
/* 线程处理 */
void* CRecvfrom::loopRecvfrom (void)
{
    while (1)
    {   
        PACKET* pPacket = new PACKET;
        int ret = recvfrom (m_UDPfd, pPacket, sizeof (PACKET), 0, NULL, NULL);
        if (ret < 0)
        {
            delete pPacket;
            cout << "recvfrom() ";
            errnos();
            continue;
        }

        g_cmdQueue.pushCmd (&pPacket->header);  // 向上抛出命令
    }  // end while

    return NULL;
}
