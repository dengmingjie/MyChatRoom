// 声明client类
#ifndef CLIENT_H
#define CLIENT_H
#include <iostream>
using namespace std;
#include <map>
#define  TCPPORT  6668  /* 服务器TCP端口号 */
typedef struct ST_SPEED
{
    long long sendTime;  // 发送时间
    long long recvTime;  // 接收时间
} SPEED;
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
    short m_port;    // 服务器TCP端口号
    string m_ip;     // 服务器IP地址
    int m_TCPSocket; // 客户端TCP套接字

    map<int, SPEED> m_speed;
};
#endif // CLIENT_H
