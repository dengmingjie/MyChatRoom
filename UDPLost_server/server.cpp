/* 实现server类 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include "server.h"
/* 构造器 */
CServer::CServer (short port, string const& ip/* = ""*/): m_port(port), m_ip(ip)
{
    m_counts = 0;
    m_maxNum = -1;
    m_lost = 0;
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
    PACKET packet;

    cout << "listening..." << endl;
	while (1) 
	{
        int ret = recvfrom (m_UDPSocket, &packet, sizeof (packet), 0, NULL, NULL);
        if (ret < 0)
        {
            cout << "recvfrom() failed！" << endl;
            continue;
        }

        ++m_counts;
        if (packet.packetNum > m_maxNum)
        {
            if (packet.packetNum - m_maxNum != 1)
            {
                for (int n = m_maxNum+1; n < packet.packetNum; ++n)
                {
                    ++m_lost;
                }
            }

            m_maxNum = packet.packetNum;  // 记录当前最大包号
        }
        else
        {
            --m_lost;
        }

        if (packet.packetNum % 10000 == 0)
        {
            cout << "recv: " << m_counts << endl;
            cout << "lost: " << m_lost << endl;
        }
        if (packet.packetNum >= 1000000)  // 100万
        {
            return;
        }
	}  // end while
}
