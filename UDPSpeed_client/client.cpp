/* 实现client类 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include "client.h"
/* 构造器 */
CClient::CClient (short port, string const& ip/* = "127.0.0.1"*/): m_port(port), m_ip(ip)
{
    m_packetNum = -1;  // 包号
}
/* 析构器 */
CClient::~CClient (void)
{
    close (m_UDPSocket);
}
/* 创建客户端UDP套接字 */
void CClient::initSocket (void)
{
    m_UDPSocket = socket (AF_INET, SOCK_DGRAM, 0);
}
/* 启动sendto模块 */
void CClient::loopSendto (void)
{
    usleep (500);
    // 初始化协议地址，用于m_TCPSocket和服务器端建立连接
    struct sockaddr_in servAddr;
    memset (&servAddr, 0, sizeof (servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons (m_port);
    servAddr.sin_addr.s_addr = inet_addr (m_ip.c_str());
    socklen_t len = sizeof (servAddr);

    PACKET packet = { 0 };
    timespec sendTime = { 0 };

    while (1)
    {
        packet.packetNum = ++m_packetNum;  // 确定包号

        int ret = sendto (m_UDPSocket, &packet, sizeof (packet), 0, (struct sockaddr*)&servAddr, len);
        if (ret < 0)
        {
            cout << "sendto() failed！" << endl;
            continue;
        }

        clock_gettime (CLOCK_REALTIME, &sendTime);  // 获取发送时间
        m_sendTime.insert (pair<int, timespec> (packet.packetNum, sendTime));

        if (packet.packetNum % 1000 == 0)
        {
            cout << "send: " << packet.packetNum << endl;
        }

        if (packet.packetNum >= 5000)
        {
            sleep (10);
            return;
        }

        //usleep (10);   // 10us
        usleep (50);   // 50us
        //usleep (100);  // 100us
        //usleep (200);  // 200us
        //usleep (500);  // 500us
    }  // end while
}
/* 启动recvfrom模块 */
void CClient::start (void)
{
    pthread_create (&m_tid, NULL, run, this);
}
/* 线程过程 */
void* CClient::run (void* arg)
{
    pthread_detach(pthread_self());
    return static_cast<CClient*> (arg)->loopRecvfrom();
}
/* 线程处理 */
void* CClient::loopRecvfrom (void)
{
    PACKET packet = { 0 };
    timespec recvTime = { 0 };
    long long speed = 0;
    long long speeds = 0;

    while (1)
    {
        int ret = recvfrom (m_UDPSocket, &packet, sizeof (packet), 0, NULL, NULL);
        if (ret < 0)
        {
            cout << "recvfrom() failed！" << endl;
            continue;
        }

        clock_gettime (CLOCK_REALTIME, &recvTime);  // 获取接收时间
        m_recvTime.insert (pair<int, timespec> (packet.packetNum, recvTime));

        if (packet.packetNum % 1000 == 0)
        {
            cout << "recv: " << packet.packetNum << endl;
        }

        if (m_recvTime.size() >= 1000)
        {
            for (map<int, timespec>::iterator it = m_recvTime.begin(); it != m_recvTime.end(); ++it)
            {
                map<int, timespec>::iterator that = m_sendTime.find (it->first);
                speed = ((it->second.tv_sec*1000000 + it->second.tv_nsec/1000) - (that->second.tv_sec*1000000 + that->second.tv_nsec/1000)) / 2;
                //speed = ((it->second.tv_nsec/1000) - (that->second.tv_nsec/1000)) / 2;
                cout << "UDP速率: " << speed << "us" << endl;
                speeds += speed;
                if (speed < 0)
                {
                    cout << "recvTime: " << it->second.tv_sec*1000000 + it->second.tv_nsec/1000 << endl;
                    cout << "sendTime: " << that->second.tv_sec*1000000 + that->second.tv_nsec/1000 << endl;
                    //cout << "recvTime: " << it->second.tv_nsec/1000 << endl;
                    //cout << "sendTime: " << that->second.tv_nsec/1000 << endl;
                }
            }
            double tmp = speeds / m_recvTime.size();
            cout << "UDP平均速率: " << tmp << "us" << endl;
            return NULL;
        }
    }
}
