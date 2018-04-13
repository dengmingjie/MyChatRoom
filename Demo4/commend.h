/* 指令处理模块 */
#ifndef  COMMEND_H
#define  COMMEND_H
#include <pthread.h>
#include "user.h"
#include "except.h"
/* 声明指令处理类 */
class CCommendDeal
{
public:
    // 启动指令处理线程
    void start (void) throw (ThreadException);
private:
    // 线程过程
    static void* run (void* arg);
    // 线程处理
    void* loopDeal (void);
	// 登录命令处理
	void logInDeal (CUser* pUser);
	// 私聊命令处理
	void pChatDeal (CUser* pUser);
	// 群聊命令处理
	void gChatDeal (CUser* pUser);
	// 登出命令处理
	void logOutDeal (CUser* pUser);
	// 整理接收缓存区
	void clearRecv (CUser* pUser);
    // 用户心跳检查
    void heartBeatCheck (void);
    pthread_t m_tid;  // 线程标识
};
#endif  // COMMEND_H
