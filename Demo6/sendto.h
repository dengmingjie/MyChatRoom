/* sendto模块 */
#ifndef  __SENDTO_H__
#define  __SENDTO_H__
#include <pthread.h>
#include "except.h"
/* 声明sendto模块类 */
class CSendto
{
public:
	// 构造器
    CSendto (int UDPfd);
	// 析构器
	~CSendto (void);
    // 启动传输文件模块
    void start (void) throw (ThreadException);
private:
    // 线程过程
    static void* run (void* arg);
    // 线程处理
    void* loopSendto (void);

    int m_UDPfd;  // 服务器UDP套接字
    pthread_t m_tid;  // 线程标识
};
#endif  // __SENDTO_H__
