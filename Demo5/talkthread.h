/* 声明会话线程类 */
#ifndef  __TALKTHREAD_H__
#define  __TALKTHREAD_H__
#include <pthread.h>
#include <map>
#include "user.h"
#include "except.h"
#define  MAXUSERS  512  /* 最大用户数量 */
#define  MAXEVENTS 256  /* 最大事件数量 */
/* 会话线程类 */
class CTalkThread 
{
public:
	// 启动会话线程
	void start (void) throw (ThreadException);
private:
	// 会话线程过程
	static void* run (void* arg);
	// 会话线程处理
	void* loopTalk (void);
    // 向客户端发送心跳
    void sendHeartBeat (CUser* pUser) throw (SendException);
	// 接收数据
    bool recvData (int userID, CUser* pUser) throw (RecvException);
    // 接收包体
    bool recvBody (int userID, CUser* pUser) throw (RecvException);
	// 发送数据
    bool sendData (int userID, CUser* pUser) throw (SendException);
    // 整理发送缓存区
    void clearSend (CUser* pUser) throw (ThreadException);
    // 标记可删除用户
    void markDelete (CUser* pUser);
    pthread_t m_tid;  // 线程标识
    int m_epollfd;
    map<int, CUser*> m_userMap;
};
#endif  // __TALKTHREAD_H__
