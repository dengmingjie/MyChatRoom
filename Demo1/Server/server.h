// 声明server类
#ifndef  __SERVER_H__
#define  __SERVER_H__
#include <string>
#include <iostream>
using namespace std;
#include "except.h"

// 服务器
class Server {
public:
	// 构造器
	Server (short port, string const& ip = "")
		throw (ServerException);
	// 析构器
	~Server (void);
	// 初始化服务器
	void initServer(void) throw (SocketException);
	// 循环监控并进行数据交互
	void loopSelect (void) throw (ServerException);
private:
	short m_port;    // 服务器端口号
	string m_ip;     // 服务器IP地址
	int m_listenfd;  // 监听套接字
	int nClient;     // 客户端的数量
	int client_fds[FD_SETSIZE];  // 会话套接字数组
	int max_fd;      // 可读集合最大有效下表
	fd_set allSet;   // 定义可读事件集合
};
#endif  // __SERVER_H__
