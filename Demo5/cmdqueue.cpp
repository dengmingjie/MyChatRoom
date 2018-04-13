/* 实现指令队列 */
#include <iostream>
using namespace std;
#include "cmdqueue.h"
#include "mutex.h"
CCmdQueue g_cmdQueue;  // 定义全局指令队列
/* 构造器 */
CCmdQueue::CCmdQueue (void)
{
	m_count = 0;  // 初始化元素数量
    pthread_mutex_init (&m_mutex, NULL);  // 初始化互斥锁
}
/* 析构器 */
CCmdQueue::~CCmdQueue (void)
{
	pthread_mutex_destroy (&m_mutex);  // 销毁互斥锁
}
/* 压入指令队列 */
void CCmdQueue::pushCmd (HEADER* pHeader)
{
    cout << "压入指令队列开始..." << endl;
    CMutex mutex (m_mutex);    // 加互斥锁
    m_cmd.push_back (pHeader); // 尾部追加
	++m_count;  // 元素数量+1
    cout << "压入指令队列完成！" << endl;
}
/* 弹出指令队列 */
void CCmdQueue::popCmd (void)
{
    cout << "弹出指令队列开始..." << endl;
    CMutex mutex (m_mutex); // 加互斥锁
    m_cmd.pop_front();      // 头部删除
	--m_count;  // 元素数量-1
    cout << "弹出指令队列完成！" << endl;
}
/* 获取首元素 */
HEADER* CCmdQueue::frontCmd (void)
{
	return m_cmd.front();
}
/* 判断队列是否为空 */
bool CCmdQueue::empty (void)
{
	return !m_count;
}
