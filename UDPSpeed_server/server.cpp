/* 实现server类 */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "server.h"
/* 构造器 */
CServer::CServer (short port, string const& ip/* = ""*/): m_port(port), m_ip(ip) {}
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
    struct sockaddr_in clieUDPAddr;
    socklen_t len = sizeof (clieUDPAddr);

    cout << "listening..." << endl;
    while (1)
    {
        int ret = recvfrom (m_UDPSocket, &packet, sizeof (packet), 0, (struct sockaddr*)&clieUDPAddr, &len);
        if (ret < 0)
        {
            cout << "recvfrom() failed！" << endl;
            continue;
        }

        if (packet.packetNum % 100 == 0)
        {
            cout << "包号：" << packet.packetNum << endl;
        }

        ret = sendto (m_UDPSocket, &packet, sizeof (packet), 0, (struct sockaddr*)&clieUDPAddr, len);
        if (ret < 0)
        {
            cout << "sendto() failed！" << endl;
            continue;
        }
    }  // end while
}
