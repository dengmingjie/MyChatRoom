/* sendto模块 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
using namespace std;
#include "sendto.h"
#include "queue.h"
#include "errnos.h"
#include "packet.h"
/* 构造器 */
CSendto::CSendto (int UDPfd)
{
    m_UDPfd = UDPfd;
}
/* 析构器 */
CSendto::~CSendto (void) {}
/* 启动sendto模块 */
void CSendto::start (void) throw (ThreadException)
{
    cout << "启动sendto模块开始..." << endl;
    int error = pthread_create (&m_tid, NULL, run, this);
    if (error)
        throw ThreadException (strerror (errno));
    cout << "启动sendto模块完成！ " << endl;
}
/* 线程过程 */
void* CSendto::run (void* arg)
{
    pthread_detach (pthread_self());
    return static_cast<CSendto*> (arg)->loopSendto();
}
/* 线程处理 */
void* CSendto::loopSendto (void)
{
    while (1)
    {
        // 判断队列是否有两个元素
        if (g_queue.istow())
        {
            // 获取UDP数据包
            PACKET* pPacket = (PACKET*)g_queue.front();
            g_queue.pop();

            // 获取用户UDP协议地址
            struct sockaddr_in* clientUDPAddr = (struct sockaddr_in*)g_queue.front();
            g_queue.pop();
            socklen_t len = sizeof (clientUDPAddr);

            int ret = sendto (m_UDPfd, &pPacket, sizeof (PACKET), 0, (struct sockaddr*)&clientUDPAddr, len);
            if (ret < 0)
            {
                delete pPacket;
                cout << "sendto() ";
                errnos();
                continue;
            }

            usleep (100);  // 100us
        }
        else
        {
            usleep (20);  // 20us
        }
    }  // end while

    return NULL;
}
