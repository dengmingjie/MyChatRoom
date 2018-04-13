/* 声明队列 */
#ifndef  __QUEUE_H__
#define  __QUEUE_H__
#include <pthread.h>
#include <list>
/* 队列 */
class CQueue
{
public:
	// 构造器
    CQueue (void);
	// 析构器
    ~CQueue (void);
    // 压入队列
    void push (void* pVoid);
    // 弹出队列
    void pop (void);
	// 获取首元素
    void* front (void);
    // 判断队列是否有两个元素
    bool istow (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
    list<void*> m_list;
	int m_count;  // 元素数量
};
extern CQueue g_queue;
#endif  // __QUEUE_H__
