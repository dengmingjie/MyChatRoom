/* 实现队列 */ 
#include <iostream>
using namespace std;
#include "queue.h"
#include "mutex.h"
CQueue g_queue;  // 定义全局队列
/* 构造器 */
CQueue::CQueue (void)
{
	m_count = 0;  // 初始化元素数量
    pthread_mutex_init (&m_mutex, NULL);  // 初始化互斥锁
}
/* 析构器 */
CQueue::~CQueue (void)
{
	pthread_mutex_destroy (&m_mutex);  // 销毁互斥锁
}
/* 压入队列 */
void CQueue::push (void* pVoid)
{
	cout << "压入队列开始..." << endl;
    CMutex mutex (m_mutex);   // 加互斥锁
    m_list.push_back (pVoid); // 尾部追加
	++m_count;  // 元素数量+1
    cout << "压入队列完成！" << endl;
}
/* 弹出队列 */
void CQueue::pop (void)
{
    cout << "弹出队列开始..." << endl;
    CMutex mutex (m_mutex); // 加互斥锁
    m_list.pop_front();     // 头部删除
	--m_count;  // 元素数量-1
    cout << "弹出队列完成！" << endl;
}
/* 获取首元素 */
void* CQueue::front (void)
{
	return m_list.front();
}
/* 判断队列是否有两个元素 */
bool CQueue::istow (void)
{
    return m_count - 2 >= 0;
}
