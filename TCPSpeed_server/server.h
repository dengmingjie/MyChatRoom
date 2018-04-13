// 声明服务器类
#ifndef SERVER_H
#define SERVER_H
#include <time.h>
#include <pthread.h>
#include <iostream>
using namespace std;
#include <map>
#define  TCPPORT  6668  /* 服务器TCP端口号 */
typedef struct ST_PACKET
{
    int packetNum;
    char data[1000];
} PACKET;
/* 服务器类 */
class CServer
{
public:
    // 构造器
    CServer (short port, string const& ip = "");
    // 析构器
    ~CServer (void);
    // 初始化服务器
    void initServer (void);
    // 功能函数
    void run (void);
private:
    short m_port;    // 服务器TCP端口号
    string m_ip;     // 服务器IP地址
    int m_TCPSocket; // 服务器TCP套接字
};
#endif // SERVER_H
