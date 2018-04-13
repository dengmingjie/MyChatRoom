/* 定义进程入口函数*/
#include "client.h"
/* 处理SIGINT信号 */
void handle (int signum) 
{
    cout << "client will quit in 2 seconds!" << endl;
    sleep (2);
    throw ClientException (strerror (errno));
}
/* 进程入口 */
int main (int argc, char* argv[]) 
{
	try 
	{
		// 向系统注册对SIGINT信号的处理函数
        signal (SIGINT, handle);
		// 创建客户端对象
        Client client (TCPPORT);
		// 连接服务器
		client.connectServer();
		// 获取内核分配的本地协议地址
		client.getLocalAddr();
        // 启动指令线程
        client.start();
        // 循环监控
        client.loopSelect();
	}
	catch (exception& ex) 
	{
		cout << ex.what() << endl;
		return -1;
	}
	return 0;
}
