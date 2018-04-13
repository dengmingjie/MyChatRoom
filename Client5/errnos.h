#ifndef  __ERRNOS_H__
#define  __ERRNOS_H__
#include <errno.h>
#include <iostream>
using namespace std;
static void errnos (void)
{
	switch (errno)
	{
    	case EAGAIN:
    		cout << "EAGAIN: 套接字已标记为非阻塞，而接收操作被阻塞或者接收超时!" << endl;
    		// 正常，不做处理
            break;
    	case EINTR:
    		cout << "EINTR: 操作被信号中断!" << endl;
    		// 正常，不做处理
    		break;
    	case EBADF:
    		cout << "EBADF: socket不是有效的套接字！" << endl;
    		// 出错，提示错误信息
    		break;
    	case EINVAL:
    		cout << "EINVAL: 传给系统的参数无效!" << endl;
    		// 出错，提示错误信息
    		break;
    	case EFAULT:
    		cout << "EFAULT: 内存空间访问出错!" << endl;
    		// 出错，提示错误信息
    		break;
    	case ENOMEM:
    		cout << "ENOMEM: 核心内存不足！" << endl;
    		// 出错，做相关处理
    		break;
    	// SOCKET_ERROR: 网络错误！
    	// ECONNREFUSE: 远程主机阻绝网络连接!
    	// ENOTCONN: 与面向连接关联的套接字尚未被连接上！
    	// ENOTSOCK: socket不是套接字！
    	// ENOBUFS: 系统的缓冲内存不足！
    	// errno == EWOULDBLOCK 正常，不做处理
	}
}
#endif  // __ERRNOS_H__
