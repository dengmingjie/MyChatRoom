/* 声明新用户队列 */
#ifndef  __NEWQUEUE_H__
#define  __NEWQUEUE_H__
#include <pthread.h>
#include <list>
#include "user.h"
/* 新用户队列 */
class CNewQueue
{
public:
	// 构造器
    CNewQueue (void);
	// 析构器
    ~CNewQueue (void);
    // 压入队列
    void pushNew (CUser* pUser);
    // 弹出队列
    void popNew (void);
	// 获取首元素
	CUser* frontNew (void);
	// 判断队列是否为空
	bool empty (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
    list<CUser*> m_new;
	int m_count;  // 元素数量
};
extern CNewQueue g_newQueue;
#endif  // __NEWQUEUE_H__
