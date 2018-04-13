/* 实现server类 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "server.h"
/* 构造器 */
CServer::CServer (short port, string const& ip/* = ""*/): m_port(port), m_ip(ip)
{
    m_maxNum = -1;
    m_counts = 0;
    m_maxDif = 0;
}
/* 析构器 */
CServer::~CServer (void) 
{
    close (m_UDPSocket);
}
/* 初始化服务器 */
void CServer::initServer (void)
{
	cout << "初始化服务器开始..." << endl;
    // 创建服务器UDP套接字
    m_UDPSocket = socket (AF_INET, SOCK_DGRAM, 0);

    // 设置端口重用
    int on = 1;
    setsockopt (m_UDPSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));

    // 初始化服务器UDP协议地址
    struct sockaddr_in servUDPAddr;
    memset (&servUDPAddr, 0, sizeof (servUDPAddr));
    servUDPAddr.sin_family = AF_INET;
    servUDPAddr.sin_port = htons (m_port);
    servUDPAddr.sin_addr.s_addr = htonl (INADDR_ANY);

    // 绑定
    bind (m_UDPSocket, (struct sockaddr*)&servUDPAddr, sizeof(servUDPAddr));
	cout << "初始化服务器完成！ " << endl;
}
/* 功能函数 */
void CServer::run (void)
{
    cout << "listening..." << endl;
    TIMES times;
    memset (&times, 0, sizeof (times));
	while (1) 
	{
        PACKET packet;
        int ret = recvfrom (m_UDPSocket, &packet, sizeof (packet), 0, NULL, NULL);
        if (ret < 0)
        {
            cout << "recvfrom() failed！" << endl;
            continue;
        }

        if (packet.packetNum > m_maxNum)
        {
            if (packet.packetNum - m_maxNum == 1)
            {
                if (packet.packetNum % 1000 == 0)
                {
                    cout << "接收正常！" << endl;
                }
            }
            else
            {
                if (packet.packetNum % 1000 == 0)
                {
                    cout << "接收乱序！" << endl;
                }
                for (int n = m_maxNum+1; n < packet.packetNum; ++n)
                {
                    gettimeofday (&times.firstTime, NULL);  // 记录当前包号乱序时间
                    m_time.insert (pair<int, TIMES> (n, times));
                }
            }

            m_maxNum = packet.packetNum;  // 记录当前最大包号
        }
        else
        {
            map<int, TIMES>::iterator it = m_time.find (packet.packetNum);
            gettimeofday (&it->second.secondTime, NULL);  // 记录当前乱序包号接收时间
            m_maxDif += (it->second.secondTime.tv_sec * 1000000 + it->second.secondTime.tv_usec) - (it->second.firstTime.tv_sec * 1000000 + it->second.firstTime.tv_usec);
            m_counts += 1;
            float tmp = m_maxDif / m_counts;
            cout << "平均乱序时间差：" << tmp << "us" << endl;
        }

        if (packet.packetNum >= 500000)  // 50万
        {
            if (m_time.empty())
            {
                cout << "未发现乱序！" << endl;
            }
            else
            {
                float tmp = m_maxDif / m_counts;
                cout << "平均乱序时间差：" << tmp << "us" << endl;
            }
            return;
        }
	}  // end while
}
