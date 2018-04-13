/* 声明待发送指令队列 */
#ifndef  __SENDQUEUE_H__
#define  __SENDQUEUE_H__
#include <pthread.h>
#include <list>
#include "sendbuf.h"
/* 待发送指令队列 */
class CSendQueue
{
public:
	// 构造器
    CSendQueue (void);
	// 析构器
    ~CSendQueue (void);
    // 压入待发送指令队列
    void pushSend (CSendBuf* pSendBuf);
    // 弹出待发送指令队列
    void popSend (void);
	// 获取首元素
	CSendBuf* frontSend (void);
	// 判断队列是否为空
	bool empty (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
    list<CSendBuf*> m_send;
	int m_count;  // 元素数量
};
extern CSendQueue g_sendQueue;
#endif  // __SENDQUEUE_H__
