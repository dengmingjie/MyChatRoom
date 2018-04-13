#include "client.h"

int main(int argc, char *argv[])
{
    // 创建客户端对象
    CClient client (UDPPORT, "219.83.160.170");
    //CClient client (UDPPORT, "192.168.199.219");
    //CClient client (UDPPORT);
    // 启动功能函数
    client.run();
    return 0;
}
