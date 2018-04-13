#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#define  PORT   8888         /* 端口号 */
#define  IPADDR "172.30.9.139"  /* IP地址 */

// 定义TCP数据包
typedef struct TAG_TCP_PACKET {
	int nLen;
	char buf[1024];
} PACKET;
char sendbuf[1024] = {0};  // 发送缓冲区
char recvbuf[1024] = {0};  // 接收缓冲区
int main(int argc, char* argv[]) {
	// 创建客户端套接字
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1) {            // -1表示出错
		perror("socket() failed");  // 输出错误原因
		return -1;
	}

	// 初始化协议地址，用于sock_fd和服务器端建立连接
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT);
	servAddr.sin_addr.s_addr = inet_addr(IPADDR);
//	inet_aton(IPADDR, &servAddr.sin_addr);

	// 向服务器发起连接请求，三次握手
	if (connect(sock_fd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
		perror("connect() failed");  // 
		close (sock_fd);  // 关闭客户端套接字
		return -1;
	}

	struct sockaddr_in localAddr;  // 本地协议地址
	char clientIP[20];  // 存储客户端IP地址
	socklen_t local_len = sizeof(localAddr);  
    memset(&localAddr, 0, sizeof(localAddr));
	// connect()连接成功后，获取内核分配的本地协议地址
	if (getsockname(sock_fd, (struct sockaddr*)&localAddr, &local_len) == -1) {
		perror("getsockname() failed");  // 
		close (sock_fd);  // 关闭客户端套接字
		return -1;
	}
	inet_ntop(AF_INET, &localAddr.sin_addr, clientIP, sizeof(clientIP));
	// 在屏幕输出IP地址和端口号
	printf("host %s:%d\n", clientIP, ntohs(localAddr.sin_port));

	fd_set rSet;  // 定义可读事件集合
	struct timeval timeout;  // 设置等待时间
	/* NULL表示无限等待，0表示轮询 */
	int max_fd;  // 集合内最大有效下标
	int stdin_fd = fileno(stdin);  // 获取标准输入的文件描述符
	if (stdin_fd > sock_fd)  // 确定集合内最大有效下标
        max_fd = stdin_fd;  
    else  
        max_fd = sock_fd;  

	// 循环监控并进行数据交互
	while (1) {
		FD_ZERO(&rSet);  // 清空集合
		FD_SET(stdin_fd, &rSet);  // 添加标准输出
		FD_SET(sock_fd, &rSet);   // 添加套接字
		timeout.tv_sec = 2;   // 秒
		timeout.tv_usec = 0;  // 毫秒
		int nReady = select(max_fd + 1, &rSet, NULL, NULL, &timeout);
		if (nReady == -1) {  // 出错
			perror("select() failed");  // 
			return -1;
		}
		if (nReady == 0)  // 超时
			continue;

		// 判断是否有消息需要接收
		if (FD_ISSET(sock_fd, &rSet)) {
			printf("message:\n");  // 测试收到消息
//			int ret = read(sock_fd, recvbuf, sizeof(recvbuf));

			PACKET packet;
			memset(&packet, 0, sizeof(packet));
			int ret = read(sock_fd, &packet, sizeof(packet));
			printf("%d: %s\n", packet.nLen, packet.buf);

			if (ret == -1) {
				perror("read() failed");  // 
				return -1;
			}
			else if (ret == 0) {  // 服务器关闭
				printf("\nserver close!\n");
				break;
			}
			
//			fputs(recvbuf, stdout);  // 输出消息
			memset(recvbuf, 0, sizeof(recvbuf));
		}

		// 判断是否有键盘输入
		if (FD_ISSET(stdin_fd, &rSet)) {
			if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) 
				break;

			PACKET packet;
			memset(&packet, 0, sizeof(packet));
			strcpy(packet.buf, sendbuf);
			packet.nLen = strlen(sendbuf);
			int rets = send(sock_fd, &packet, sizeof(packet), 0);
			printf("packet.buf:%s\n", packet.buf);  // 测试
/*
			int rets = send(sock_fd, sendbuf, sizeof(sendbuf), 0);  // 发送
			printf("sendbuf:%s\n", sendbuf);  // 测试*/

			if (rets < 0) {  // SOCKET_ERROR
				perror("send() failed");  // 
				return -1;
			}
			else if (rets == 0) {  // 服务器关闭
				printf("\nserver close!\n");
				break;
			}

			memset(sendbuf, 0, sizeof(sendbuf));
		}
	}  // end while

	// 关闭客户端套接字，四次分手
	close(sock_fd);

	return 0;
}
