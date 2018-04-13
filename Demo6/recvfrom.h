/* recvfrom模块 */
#ifndef  __RECVFROM_H__
#define  __RECVFROM_H__
#include <pthread.h>
#include "except.h"
/* 声明recvfrom模块类 */
class CRecvfrom
{
public:
	// 构造器
    CRecvfrom (int UDPfd);
	// 析构器
	~CRecvfrom (void);
    // 启动recvfrom模块
    void start (void) throw (ThreadException);
private:
    // 线程过程
    static void* run (void* arg);
    // 线程处理
    void* loopRecvfrom (void);

    int m_UDPfd;  // 服务器UDP套接字
    pthread_t m_tid;  // 线程标识
};
#endif  // __RECVFROM_H__
