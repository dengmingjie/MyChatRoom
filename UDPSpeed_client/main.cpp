#include "client.h"

int main(int argc, char *argv[])
{
    // 创建客户端对象
    //CClient client(UDPPORT, "219.83.160.170");
    CClient client(UDPPORT);
    // 创建客户端UDP套接字
    client.initSocket();
    // 启动recvfrom模块
    client.start();
    // 启动sendto模块
    client.loopSendto();
    return 0;
}
