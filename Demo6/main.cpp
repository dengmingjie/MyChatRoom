/* 定义进程入口函数 */
#include "server.h"
#include "talkthread.h"
#include "recvfrom.h"
#include "sendto.h"
#include "commend.h"
/* 进程入口 */
int main (int argc, char* argv[])
{
	try 
	{
		// 创建服务器对象		
        CServer server (TCPPORT);
		// 初始化服务器
		server.initServer();
		// 启动监听线程
		server.start();
		// 创建数据I/O模块
		CTalkThread talk;
		// 启动数据I/O模块
		talk.start();
        // 创建recvfrom模块
        CRecvfrom recvfrom (server.getUDPfd());
        // 启动recvfrom模块
        recvfrom.start();
        // 创建sendto模块
        CSendto sendto (server.getUDPfd());
        // 启动sendto模块
        sendto.start();
        // 创建指令处理模块
        CCommendDeal deal;
        // 启动指令处理模块
        deal.start();

		// 主线程阻塞
		char str[10] = {0};
		while (strcmp (str, "stop\n"))
		{
            sleep (2);
            fgets (str, sizeof (str), stdin);
		}
		cout << "server close!" << endl;
	}
	catch (exception& ex) 
	{
		cout << ex.what() << endl;
		return -1;
	}
	return 0;
}
