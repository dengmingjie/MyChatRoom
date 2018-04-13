/* 实现新用户队列 */
#include <iostream>
using namespace std;
#include "newqueue.h"
#include "mutex.h"
CNewQueue g_newQueue;  /* 定义全局新用户队列 */
/* 构造器 */
CNewQueue::CNewQueue (void)
{
	m_count = 0;  // 初始化元素数量
    pthread_mutex_init (&m_mutex, NULL);  // 初始化互斥锁
}
/* 析构器 */
CNewQueue::~CNewQueue (void)
{
    pthread_mutex_destroy (&m_mutex);  // 销毁互斥锁
}
/* 压入新用户队列 */
void CNewQueue::pushNew (CUser* pUser)
{
    cout << "压入新用户队列开始..." << endl;
    CMutex mutex (m_mutex);  // 加互斥锁
    m_new.push_back (pUser); // 尾部追加
	++m_count;  // 元素数量+1
    cout << "压入新用户队列完成！" << endl;
}
/* 弹出新用户队列 */
void CNewQueue::popNew (void)
{
    cout << "弹出新用户队列开始..." << endl;
    CMutex mutex (m_mutex); // 加互斥锁
    m_new.pop_front ();     // 头部删除
	--m_count;  // 元素数量-1
    cout << "弹出新用户队列完成！" << endl;
}
/* 获取首元素 */
CUser* CNewQueue::frontNew (void)
{
	return m_new.front();
}
/* 判断队列是否为空 */
bool CNewQueue::empty (void)
{
	return !m_count;
}
