/* 声明队列 */
#ifndef  __QUEUE_H__
#define  __QUEUE_H__
#include <pthread.h>
#include <list>
#include "user.h"
/* 队列 */
class CQueue
{
public:
	// 构造器
    CQueue (void);
	// 析构器
    ~CQueue (void);
    // 压入队列
    void push (CUser* pUser);
    // 弹出队列
    void pop (void);
	// 获取首元素
	CUser* front (void);
	// 判断队列是否为空
	bool empty (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
    list<CUser*> m_userList;  // 用户链表
};
extern CQueue g_queue;
#endif  // __QUEUE_H__
