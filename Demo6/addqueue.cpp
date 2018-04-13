/* 实现新增用户队列 */ 
#include <iostream>
using namespace std;
#include "addqueue.h"
#include "mutex.h"
CAddQueue g_addQueue;  // 定义全局新增用户队列
/* 构造器 */
CAddQueue::CAddQueue (void)
{
	m_count = 0;  // 初始化元素数量
    pthread_mutex_init (&m_mutex, NULL);  // 初始化互斥锁
}
/* 析构器 */
CAddQueue::~CAddQueue (void)
{
	pthread_mutex_destroy (&m_mutex);  // 销毁互斥锁
}
/* 压入新增用户队列 */
void CAddQueue::pushAdd (CUser* pUser)
{
	cout << "压入新增用户队列开始..." << endl;
    CMutex mutex (m_mutex);  // 加互斥锁
    m_add.push_back (pUser); // 尾部追加
	++m_count;  // 元素数量+1
    cout << "压入新增用户队列完成！" << endl;
}
/* 弹出新增用户队列 */
void CAddQueue::popAdd (void)
{
    cout << "弹出新增用户队列开始..." << endl;
    CMutex mutex (m_mutex); // 加互斥锁
    m_add.pop_front();      // 头部删除
	--m_count;  // 元素数量-1
    cout << "弹出新增用户队列完成！" << endl;
}
/* 获取首元素 */
CUser* CAddQueue::frontAdd (void)
{
	return m_add.front();
}
/* 判断队列是否为空 */
bool CAddQueue::empty (void)
{
	return !m_count;
}
