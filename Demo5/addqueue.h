/* 声明新增用户队列 */
#ifndef  __ADDQUEUE_H__
#define  __ADDQUEUE_H__
#include <pthread.h>
#include <list>
#include "user.h"
/* 新增用户队列 */
class CAddQueue
{
public:
	// 构造器
    CAddQueue (void);
	// 析构器
    ~CAddQueue (void);
    // 压入新增用户队列
    void pushAdd (CUser* pUser);
    // 弹出新增用户队列
    void popAdd (void);
	// 获取首元素
	CUser* frontAdd (void);
	// 判断队列是否为空
	bool empty (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
    list<CUser*> m_add;
	int m_count;  // 元素数量
};
extern CAddQueue g_addQueue;
#endif  // __ADDQUEUE_H__
