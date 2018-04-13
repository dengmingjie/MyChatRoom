// 声明client类
#ifndef CLIENT_H
#define CLIENT_H
#include <iostream>
using namespace std;
#define  UDPPORT  6665  /* 服务器UDP端口号 */
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
    // 启动功能函数
    void run (void);
private:
    short m_port;    // 服务器端口号
    string m_ip;     // 服务器IP地址
    int m_UDPSocket; // 客户端UDP套接字

    int m_packetNum; // 包号
};
#endif // CLIENT_H
