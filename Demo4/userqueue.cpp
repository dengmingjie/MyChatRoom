/* 实现用户队列 */ 
#include <iostream>
using namespace std;
#include "userqueue.h"
#include "mutex.h"
CUserQueue g_userQueue;  // 定义全局用户队列
/* 构造器 */
CUserQueue::CUserQueue (void)
{
    pthread_mutex_init (&m_mutex, NULL);  // 初始化互斥锁
}
/* 析构器 */
CUserQueue::~CUserQueue (void)
{
	pthread_mutex_destroy (&m_mutex);  // 销毁互斥锁
	for (map<int, CUser*>::iterator it = g_userMap.begin(); it != g_userMap.end(); ++it)
	{
		delUser (it->second);  // 删除用户
		cout << "堆内存已释放！！！" << endl;
	}
}
/* 压入用户队列 */
void CUserQueue::pushUser (CUser* pUser)
{
	cout << "压入用户队列开始..." << endl;
    CMutex mutex (m_mutex);   // 加互斥锁
    m_user.push_back (pUser); // 尾部追加
    cout << "压入用户队列完成！" << endl;
}
/* 弹出用户队列 */
void CUserQueue::popUser (void)
{
    cout << "弹出用户队列开始..." << endl;
    CMutex mutex (m_mutex); // 加互斥锁
    m_user.pop_front ();    // 头部删除
    cout << "弹出用户队列完成！" << endl;
}
/* 获取首元素 */
CUser* CUserQueue::frontUser (void)
{
	return m_user.front();
}
/* 判断队列是否为空 */
bool CUserQueue::empty (void)
{
	return m_user.empty();
}
