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
    close (m_TCPSocket);
}
/* 初始化服务器 */
void CServer::initServer (void)
{
	cout << "初始化服务器开始..." << endl;
    // 创建服务器监听套接字
    m_TCPSocket = socket (AF_INET, SOCK_STREAM, 0);

    // 设置端口重用
    int on = 1;
    setsockopt (m_TCPSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // 初始化服务器协议地址
    struct sockaddr_in servAddr;
    memset (&servAddr, 0, sizeof (servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons (m_port);
    servAddr.sin_addr.s_addr = htonl (INADDR_ANY);

    // 绑定
    bind (m_TCPSocket, (struct sockaddr*)&servAddr, sizeof(servAddr));

    // 监听套接字
    listen (m_TCPSocket, 5);
	cout << "初始化服务器完成！ " << endl;
}
/* 功能函数 */
void CServer::run (void)
{
listen:
    cout << "listening..." << endl;
    struct sockaddr_in clieAddr;
    socklen_t clieAddr_len = sizeof (clieAddr);
    int connfd = accept (m_TCPSocket, (struct sockaddr*)&clieAddr, &clieAddr_len);

    int num = 0;

    while (1)
    {
        int ret = recv (connfd, &num, sizeof (num), 0);
        if (ret < 0)
        {
            cout << "recv() failed！" << endl;
            continue;
        }

        if (num % 10 == 0)
        {
            cout << "num: " << num << endl;
        }

        ret = send (connfd, &num, sizeof (num), 0);
        if (ret < 0)
        {
            cout << "send() failed！" << endl;
            continue;
        }

        if (num >= 10000)
        {
            goto listen;
        }
    }  // end while
}
