#include "server.h"

int main(int argc, char *argv[])
{
    // 创建服务器对象
    CServer server (UDPPORT);
    // 初始化服务器
    server.initServer();
    // 启动功能函数
    server.run();
    return 0;
}
