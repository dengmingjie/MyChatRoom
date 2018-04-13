/* 声明指令队列 */
#ifndef  __CMDQUEUE_H__
#define  __CMDQUEUE_H__
#include <pthread.h>
#include <list>
#include "user.h"
/* 指令队列 */
class CCmdQueue
{
public:
	// 构造器
    CCmdQueue (void);
	// 析构器
    ~CCmdQueue (void);
    // 压入指令队列
    void pushCmd (HEADER* pHeader);
    // 弹出指令队列
    void popCmd (void);
	// 获取首元素
	HEADER* frontCmd (void);
	// 判断队列是否为空
	bool empty (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
    list<HEADER*> m_cmd;
	int m_count;  // 元素数量
};
extern CCmdQueue g_cmdQueue;
#endif  // __CMDQUEUE_H__
