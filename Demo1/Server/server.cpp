// 实现server类
#include <stdio.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <arpa/inet.h>  
#include <netinet/in.h>  
#include <string.h> 
#include "server.h"
#include "packet.h"

// 构造器
Server::Server (short port, string const& ip/* = ""*/)
	throw (ServerException): m_port(port), m_ip(ip) {
	for (int i = 0; i < FD_SETSIZE; i++) 
		client_fds[i] = -1;
}
// 析构器
Server::~Server (void) {
	// 关闭所有套接字
	for (int i = 0; i <= FD_SETSIZE; i++) {
		if (client_fds[i] > 0) {
			close(client_fds[i]);
			cout << "connfd " << client_fds[i] << " is closed..." << endl;
		}
	}
}
// 初始化服务器
void Server::initServer(void) throw (SocketException) {
	cout << "初始化服务器开始..." << endl;
	// 创建服务器监听套接字
	m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == m_listenfd)  // -1表示出错，抛异常
		throw SocketException (strerror (errno));
	
	// 设置端口重用
	int on = 1;
	if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
		throw SocketException (strerror (errno));

	// 初始化服务器协议地址
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(m_port);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 绑定
	if (bind(m_listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		throw SocketException (strerror (errno));

	// 监听套接字
	if (listen(m_listenfd, 1024) == -1)
		throw SocketException (strerror (errno));

	cout << "初始化服务器完成！ " << endl;
}
// 循环监控并进行数据交互
void Server::loopSelect (void) throw (ServerException) {
	struct sockaddr_in clieAddr;
	socklen_t clieAddr_len = sizeof(clieAddr);
	fd_set rSet;  // 定义可读事件集合
	FD_ZERO(&rSet);  // 清空结合
	FD_ZERO(&allSet);  // 清空结合
	FD_SET(m_listenfd, &allSet);  // 添加监听描述符
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */
	int max_fd = m_listenfd;  // 可读集合最大有效下标
	
	cout << "listening..." << endl;
	// 循环监控并进行数据交互
	while (1) {
		rSet = allSet;
		timeout.tv_sec = 0;   // 秒
		timeout.tv_usec = 0;  // 微秒
		int nReady = select(max_fd + 1, &rSet, NULL, NULL, &timeout);  // 轮询
		if (-1 == nReady) {
			if (errno == EINTR)
				continue;
			throw ServerException (strerror (errno));
		}
		if ( 0 == nReady)  // 超时
			continue;

		// 判断是否有客户端请求连接
		if (FD_ISSET(m_listenfd, &rSet)) {
			int connfd = accept(m_listenfd, (struct sockaddr*)&clieAddr, &clieAddr_len);  // 不会阻塞
			if (-1 == connfd) 
				throw ServerException (strerror (errno));

			// 添加到会话套接字数组
			int i = 0;
			for (i = 0; i < FD_SETSIZE; ++i) {
				if (client_fds[i] < 0) {
					client_fds[i] = connfd;  // 添加
					nClient++;  // 客户端数量+1
					if (connfd > max_fd)
						max_fd = connfd;
					break;
				}
			}  // end for

			// 客户端数量达到最大，则服务器退出
			if (i == FD_SETSIZE) 
				throw ServerException (strerror (errno));

			// 输出客户端IP地址和端口号
			cout << "new client: " << inet_ntoa(clieAddr.sin_addr) << ':' << ntohs(clieAddr.sin_port) << endl;
			cout << "clients: " << nClient << endl;

			// 添加会话套接字
			FD_SET(connfd, &allSet);
		}

		// 判断是否有客户端发来消息
		for (int i = 0; i <= nClient; i++) {
			if (-1 == client_fds[i]) 
				continue;

			// 若有消息
			if (FD_ISSET(client_fds[i], &rSet)) {
//				int ret = read(client_fds[i], recvbuf, sizeof(recvbuf));

				PACKET packet;
				memset(&packet, 0, sizeof(packet));
				int ret = read(client_fds[i], &packet, sizeof(packet));
					
				if (-1 == ret)
					throw RecvException (strerror (errno));
				else if (0 == ret) {  // 客户端关闭
					cout << endl << "client close!" << endl;
					FD_CLR(client_fds[i], &allSet);  // 将其套接字清除
					if (client_fds[i] > max_fd) 
						max_fd--;
					nClient--;  // 客户端数量-1
					cout << "clients: " << nClient << endl;
					close(client_fds[i]);  // 关闭其会话套接字
					client_fds[i] = -1;  // 置为-1
					continue;
				}

				cout << "message: " << endl;  // 测试收到消息
				cout << packet.msg_len << ": " << packet.msg_buf << endl;

				// 向所有用户转发消息
				for (int i = 0; i <= nClient; i++) {
					if (-1 == client_fds[i])
						continue;
					int ret = send(client_fds[i], &packet, sizeof(packet), 0);  // 发送
					if (ret < 0)  // SOCKET_ERROR
						throw SendException (strerror (errno));
					else if (0 == ret) {  // 客户端关闭
						cout << endl << "client close!" << endl;
						FD_CLR(client_fds[i], &allSet);  // 将其套接字清除
						if (client_fds[i] > max_fd) 
							max_fd--;
						nClient--;  // 客户端数量-1
						cout << "clients: " << nClient << endl;
						close(client_fds[i]);  // 关闭其会话套接字
						client_fds[i] = -1;  // 置为-1
					}
				}  // end for
			}  // end if
		}  // end for
	}  // end while
}
