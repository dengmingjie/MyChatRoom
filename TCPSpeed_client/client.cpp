/* 实现client类 */
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <string.h>
#include "client.h"
/* 构造器 */
CClient::CClient (short port, string const& ip/* = "127.0.0.1"*/): m_port(port), m_ip(ip) {}
/* 析构器 */
CClient::~CClient (void)
{
    close (m_TCPSocket);
}
/* 启动功能函数*/
void CClient::run (void)
{
    // 创建客户端TCP套接字
    m_TCPSocket = socket (AF_INET, SOCK_STREAM, 0);

    // 初始化协议地址，用于m_TCPSocket和服务器端建立连接
    struct sockaddr_in servAddr;
    memset (&servAddr, 0, sizeof (servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons (m_port);
    servAddr.sin_addr.s_addr = inet_addr (m_ip.c_str());

CONNECT:
    // 向服务器发起连接请求，三次握手
    int error = connect (m_TCPSocket, (struct sockaddr*)&servAddr, sizeof (servAddr));
    if (error < 0)
    {
        cout << "连接出错！正在重新连接..." << endl;
        sleep (1);
        goto CONNECT;
    }

    int num = -1;
    timeval sendTime;
    timeval recvTime;
    SPEED speed;
    memset (&speed, 0, sizeof (speed));

    while (1)
    {
        //usleep (50);   // 50us
        //usleep (100);  // 100us
        //usleep (200);  // 200us
        usleep (500);  // 500us

        ++num;
        int ret = send (m_TCPSocket, &num, sizeof (num), 0);
        if (ret < 0)
        {
            cout << "send() failed！" << endl;
            continue;
        }

        if (num % 10 == 0)
        {
            cout << "send: " << num << endl;
        }

        gettimeofday (&sendTime, NULL);
        speed.sendTime = sendTime.tv_sec * 1000000 + sendTime.tv_usec;  // 获取发送时间us
        m_speed.insert (pair<int, SPEED> (num, speed));

        ret = recv (m_TCPSocket, &num, sizeof (num), 0);
        if (ret < 0)
        {
            cout << "recv() failed！" << endl;
            continue;
        }

        if (num % 10 == 0)
        {
            cout << "recv: " << num << endl;
        }

        gettimeofday (&recvTime, NULL);
        map<int, SPEED>::iterator it = m_speed.find (num);
        it->second.recvTime = recvTime.tv_sec * 1000000 + recvTime.tv_usec;  // 获取接收时间us

        if (num >= 10000)
        {
            long long speed = 0;
            long n = 0;
            for (map<int, SPEED>::iterator it = m_speed.begin(); it != m_speed.end(); ++it)
            {
                if (it->second.recvTime != 0 && it->second.sendTime != 0)
                {
                    speed += (it->second.recvTime - it->second.sendTime) / 2;
                    ++n;
                }
            }
            double tmp = speed / n;
            cout << "TCP速率: " << tmp << "us" << endl;
            return;
        }
    }  // end while
}
