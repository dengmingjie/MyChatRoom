/* 指令处理模块 */
#ifndef  COMMEND_H
#define  COMMEND_H
#include <pthread.h>
#include <map>
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
    // 登录指令处理
    void logInDeal (HEADER* pHeader);
    // 私聊指令处理
    void pChatDeal (HEADER* pHeader);
    // 群聊指令处理
    void gChatDeal (HEADER* pHeader);
    // 登出指令处理
    void logOutDeal (HEADER* pHeader);
    // 心跳指令处理
    void heartDeal (HEADER* pHeader);
    // 非法指令处理
    void otherDeal (HEADER* pHeader);
	// 整理接收缓存区
    void clearRecv (HEADER* pHeader);
    // 用户心跳检查
    void heartBeatCheck (void);

    pthread_t m_tid;  // 线程标识
    map<int, CUser*> m_userMap;
};
#endif  // COMMEND_H
