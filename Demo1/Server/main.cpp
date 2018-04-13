// 定义进程入口函数
#include "server.h"

// 进程入口
int main(int argc, char* argv[]) {
	try {
		// 创建服务器对象		
		Server server(8888);
		// 初始化服务器
		server.initServer();
		// 循环监控并进行数据交互
		server.loopSelect();
	}
	catch (exception& ex) {
		cout << ex.what () << endl;
		return -1;
	}
	return 0;
}
