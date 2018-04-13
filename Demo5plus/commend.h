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
    void logInDeal (CRecvBuf* pRecvBuf);
    // 私聊指令处理
    void pChatDeal (CRecvBuf* pRecvBuf);
    // 群聊指令处理
    void gChatDeal (CRecvBuf* pRecvBuf);
    // 传输文件指令处理
    void transFileDeal (CRecvBuf* pRecvBuf);
	// yes/no指令处理
    void yesnoDeal (CRecvBuf* pRecvBuf);
	// 传输文件中指令处理
    void transIngDeal (CRecvBuf* pRecvBuf);
	// 传输文件失败处理
    void transFailDeal (CUser* pUser, comm cmd);
    // 登出指令处理
    void logOutDeal (CRecvBuf* pRecvBuf);
    // 心跳指令处理
    void heartDeal (CRecvBuf* pRecvBuf);
    // 非法指令处理
    void otherDeal (CRecvBuf* pRecvBuf);
    // 用户心跳检查
    void heartBeatCheck (void);

    pthread_t m_tid;  // 线程标识
    map<int, CUser*> m_userMap;
};
#endif  // COMMEND_H
