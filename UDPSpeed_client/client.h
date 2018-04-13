// 声明client类
#ifndef CLIENT_H
#define CLIENT_H
#include <time.h>
#include <iostream>
using namespace std;
#include <map>
#define  UDPPORT  6667  /* 服务器UDP端口号 */
typedef struct ST_PACKET
{
    int packetNum;
    char data[1000];
} PACKET;
/* client类 */
class CClient
{
public:
    // 构造器
    CClient (short port, string const& ip = "127.0.0.1");
    // 析构器
    ~CClient (void);
    // 创建客户端UDP套接字
    void initSocket (void);
    // 启动sendto模块
    void loopSendto (void);
    // 启动recvfrom模块
    void start (void);
private:
    // 线程过程
    static void* run (void* arg);
    // 线程处理
    void* loopRecvfrom (void);

    short m_port;    // 服务器端口号
    string m_ip;     // 服务器IP地址
    int m_UDPSocket; // 客户端UDP套接字
    pthread_t m_tid; // 线程标识

    int m_packetNum; // 包号
    map<int, timespec> m_sendTime;  // 发送时间
    map<int, timespec> m_recvTime;  // 接收时间
};
#endif // CLIENT_H
