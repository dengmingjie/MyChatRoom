/* 声明用户队列 */
#ifndef  __USERQUEUE_H__
#define  __USERQUEUE_H__
#include <pthread.h>
#include <list>
#include <map>
#include "user.h"
/* 用户队列 */
class CUserQueue
{
public:
	// 构造器
    CUserQueue (void);
	// 析构器
    ~CUserQueue (void);
    // 压入用户队列
    void pushUser (CUser* pUser);
    // 弹出用户队列
    void popUser (void);
	// 获取首元素
	CUser* frontUser (void);
	// 判断队列是否为空
	bool empty (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
    list<CUser*> m_user;
};
extern CUserQueue g_userQueue;
extern map<int, CUser*> g_userMap;

/* 删除用户 */
void delUser (CUser* pUser);
#endif  // __USERQUEUE_H__
