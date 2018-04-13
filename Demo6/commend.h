/* 指令处理模块 */
#ifndef  __COMMEND_H__
#define  __COMMEND_H__
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
    // 传输文件指令处理
    void transFileDeal (HEADER* pHeader);
    // 传输文件中指令处理
    void transFileIngDeal (HEADER* pHeader);
    // 接收文件完成指令处理
    void transFileOKDeal (HEADER* pHeader);
	// yes/no指令处理
    void yesnoDeal (HEADER* pHeader);
	// 传输文件失败处理
    void transFailDeal (CUser* pUser, comm cmd);
    // 登出指令处理
    void logOutDeal (HEADER* pHeader);
    // 心跳指令处理
    void heartDeal (HEADER* pHeader);
    // 非法指令处理
    void otherDeal (HEADER* pHeader);
    // 用户心跳检查
    void heartBeatCheck (void);

    pthread_t m_tid;  // 线程标识
    map<int, CUser*> m_userMap;
};
#endif  // __COMMEND_H__
