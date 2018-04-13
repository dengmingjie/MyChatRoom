// 实现client类
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>
#include <iostream>
using namespace std;
#include "client.h"
#include "packet.h"

// 构造器
Client::Client(short port, string const& ip/* = "127.0.0.1"*/) 
	throw (ClientException): m_port(port), m_ip(ip) {}
// 析构器
Client::~Client(void) {
	close(m_sockfd);  // 关闭客户端套接字
	cout << "m_sockfd is closed..." << endl;  // 测试
}
// 连接服务器
void Client::connectServer (void) throw (SocketException) {
	cout << "连接服务器开始..." << endl;
	// 创建客户端套接字
	m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == m_sockfd)  // -1表示出错，抛异常 
		throw SocketException (strerror (errno));

	// 初始化协议地址，用于m_sockfd和服务器端建立连接
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(m_port);
	servAddr.sin_addr.s_addr = inet_addr(m_ip.c_str());
//	inet_aton(m_ip.c_str(), &servAddr.sin_addr);

	// 向服务器发起连接请求，三次握手
	int ret = connect(m_sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr));
	if (-1 == ret) { 
		throw SocketException (strerror (errno));
	}
	cout << "连接服务器完成！ " << endl;
}
// 连接成功后，获取内核分配的本地协议地址
void Client::getLocalAddr (void) throw (ClientException) {
	cout << "获取本地协议地址开始..." << endl;
	struct sockaddr_in localAddr;  // 本地协议地址
	char clientIP[20];  // 存储客户端IP地址
	socklen_t local_len = sizeof(localAddr);  
    memset(&localAddr, 0, sizeof(localAddr));
	// 获取内核分配的本地协议地址
	if (getsockname(m_sockfd, (struct sockaddr*)&localAddr, &local_len) == -1) {
		close (m_sockfd);  // 关闭客户端套接字
		throw SocketException (strerror (errno));
	}
	inet_ntop(AF_INET, &localAddr.sin_addr, clientIP, sizeof(clientIP));
	// 在屏幕输出IP地址和端口号
	cout << "host " << clientIP << ':' << ntohs(localAddr.sin_port) << endl;
	cout << "获取本地协议地址完成！ " << endl;
}
// 循环监控并进行数据交互
void Client::loopSelect (void) throw (ClientException) {
	fd_set rSet;  // 定义可读事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */
	int max_fd;  // 集合内最大有效下标
	m_stdinfd = fileno(stdin);  // 获取标准输入的文件描述符
	if (m_stdinfd > m_sockfd)  // 确定集合内最大有效下标
		max_fd = m_stdinfd;  
    else  
	    max_fd = m_sockfd;

	// 循环监控并进行数据交互
	while (1) {
		FD_ZERO(&rSet);  // 清空集合
		FD_SET(m_stdinfd, &rSet);  // 添加标准输出
		FD_SET(m_sockfd, &rSet);   // 添加套接字
		timeout.tv_sec = 0;   // 秒
		timeout.tv_usec = 0;  // 毫秒
		int nReady = select(max_fd + 1, &rSet, NULL, NULL, &timeout);  // 轮询
		if (-1 == nReady)  // 出错
			throw ClientException (strerror (errno));
		if ( 0 == nReady)  // 超时
			continue;

		// 判断是否有消息需要接收
		if (FD_ISSET(m_sockfd, &rSet)) {
			if (!recvData())   // 接收数据
				break;
		}
		// 判断是否有键盘输入
		if (FD_ISSET(m_stdinfd, &rSet)) {
			if (!sendData())  // 发送数据
				break;
		}
	}  // end while
}
// 发送数据
bool Client::sendData (void) throw (SendException) {
	memset(sendbuf, 0, sizeof(sendbuf));
	if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) 
		return false;
/*
	PACKET packet;
	memset(&packet, 0, sizeof(packet));
	strcpy(packet.msg_buf, sendbuf);
	packet.msg_len = strlen(sendbuf);
	int ret = send(m_sockfd, &packet, sizeof(packet), 0);  // 发送
	printf("packet.msg_buf: %s\n", packet.msg_buf);  // 测试
*/
    int ret = send(m_sockfd, sendbuf, strlen(sendbuf) + 1, 0);  // 发送
	printf("sendbuf: %s\n", sendbuf);  // 测试

	if (ret < 0)  // SOCKET_ERROR
		throw SendException (strerror (errno));
	else if (0 == ret) {  // 服务器关闭
		cout << endl << "server close!" << endl;
		return false;
	}
	return true;
}
// 接收数据
bool Client::recvData (void) throw (RecvException) {
	memset(recvbuf, 0, sizeof(recvbuf));
	int ret = read(m_sockfd, recvbuf, sizeof(recvbuf));
/*
	PACKET packet;
	memset(&packet, 0, sizeof(packet));
	int ret = read(m_sockfd, &packet, sizeof(packet));
*/	
	if (-1 == ret)
		throw RecvException (strerror (errno));
	else if (0 == ret) {  // 服务器关闭
		cout << endl << "server close!" << endl;
		return false;
	}

	cout << "message: " << endl;  // 测试收到消息
//	cout << packet.msg_len << ": " << packet.msg_buf << endl;

	fputs(recvbuf, stdout);  // 输出消息
	return true;
}
