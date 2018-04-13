// 定义进程入口函数
#include "server.h"

// 进程入口
int main(int argc, char* argv[]) 
{
	try 
	{
		// 创建服务器对象		
		Server server(8888);
		// 初始化服务器
		server.initServer();
		// 启动监听线程
		server.start();
		// 创建会话线程
		TalkThread talk;
		// 启动会话线程
		talk.start();
        // 主线程阻塞
        char str[10] = {0};
        while (strcmp (str, "stop\n"))
        {
            sleep(3);
            fgets(str, sizeof (str), stdin);
        }
        cout << "server close!" << endl << endl;
	}
	catch (exception& ex) 
	{
		cout << ex.what () << endl;
		return -1;
	}
	return 0;
}
