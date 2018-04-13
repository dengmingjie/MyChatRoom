/* 实现待发送指令队列 */
#include <iostream>
using namespace std;
#include "sendqueue.h"
#include "mutex.h"
CSendQueue g_sendQueue;  // 定义待发送指令队列
/* 构造器 */
CSendQueue::CSendQueue (void)
{
	m_count = 0;  // 初始化元素数量
    pthread_mutex_init (&m_mutex, NULL);  // 初始化互斥锁
}
/* 析构器 */
CSendQueue::~CSendQueue (void)
{
	pthread_mutex_destroy (&m_mutex);  // 销毁互斥锁
}
/* 压入待发送指令队列 */
void CSendQueue::pushSend (CSendBuf* pSendBuf)
{
    cout << "压入待发送指令队列..." << endl;
    CMutex mutex (m_mutex);      // 加互斥锁
    m_send.push_back (pSendBuf); // 尾部追加
	++m_count;  // 元素数量+1
    cout << "压入待发送指令队列！" << endl;
}
/* 弹出待发送指令队列 */
void CSendQueue::popSend (void)
{
    cout << "弹出待发送指令队列..." << endl;
    CMutex mutex (m_mutex);  // 加互斥锁
    m_send.pop_front();      // 头部删除
	--m_count;  // 元素数量-1
    cout << "弹出待发送指令队列！" << endl;
}
/* 获取首元素 */
CSendBuf* CSendQueue::frontSend (void)
{
	return m_send.front();
}
/* 判断队列是否为空 */
bool CSendQueue::empty (void)
{
	return !m_count;
}
