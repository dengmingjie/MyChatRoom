/* 实现client类 */
#include <stdio.h>
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
    m_packetNum = -1; // 包号
}
/* 析构器 */
CClient::~CClient (void)
{
    close (m_UDPSocket);
}
/* 启动功能函数*/
void CClient::run (void)
{
    // 创建客户端UDP套接字
    m_UDPSocket = socket (AF_INET, SOCK_DGRAM, 0);

    // 初始化协议地址，用于m_TCPSocket和服务器端建立连接
    struct sockaddr_in servAddr;
    memset (&servAddr, 0, sizeof (servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons (m_port);
    servAddr.sin_addr.s_addr = inet_addr (m_ip.c_str());
    socklen_t len = sizeof (servAddr);

    PACKET packet;

    while (1)
    {
        packet.packetNum = ++m_packetNum;  // 确定包号

        int ret = sendto (m_UDPSocket, &packet, sizeof (packet), 0, (struct sockaddr*)&servAddr, len);
        if (ret < 0)
        {
            cout << "sendto() failed！" << endl;
            continue;
        }

        if (packet.packetNum % 10000 == 0)
        {
            cout << "send: " << packet.packetNum << endl;
        }

        //usleep (50);   // 50us    6467/500w
        //usleep (100);  // 100us    85/300w
        usleep (200);  // 200us
        //usleep (500);  // 500us

        if (packet.packetNum >= 1000000)  // 100万
        {
            return;
        }
    }  // end while
}
