/* 实现临时队列 */
#include <iostream>
using namespace std;
#include "queue.h"
#include "mutex.h"
CQueue g_queue;  // 定义全局队列
/* 构造器 */
CQueue::CQueue (void)
{
    pthread_mutex_init (&m_mutex, NULL);  // 初始化互斥锁
}
/* 析构器 */
CQueue::~CQueue (void)
{
    pthread_mutex_destroy (&m_mutex);  // 销毁互斥锁
}
/* 压入队列 */
void CQueue::push (CUser* pUser)
{
    cout << "压入队列开始..." << endl;
    CMutex mutex (m_mutex);       // 加互斥锁
    m_userList.push_back (pUser); // 尾部追加
    cout << "压入队列完成！" << endl;
}
/* 弹出队列 */
void CQueue::pop (void)
{
    cout << "弹出队列开始..." << endl;
    CMutex mutex (m_mutex);  // 加互斥锁
    m_userList.pop_front (); // 头部删除
    cout << "弹出队列完成！" << endl;
}
/* 获取首元素 */
CUser* CQueue::front (void)
{
	return m_userList.front();
}
/* 判断队列是否为空 */
bool CQueue::empty (void)
{
	return m_userList.empty();
}
