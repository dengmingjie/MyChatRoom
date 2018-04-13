/* 指令处理模块 */
#ifndef  __COMMEND_H__
#define  __COMMEND_H__
#include <pthread.h>
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
    // 登录成功指令处理
    void logInSuccessDeal (HEADER* pHeader);
    // 私聊指令处理
    void pChatDeal (HEADER* pHeader);
    // 群聊指令处理
    void gChatDeal (HEADER* pHeader);
    // 私聊失败指令处理
    void pChatFailureDeal (HEADER* pHeader);
    // 传输文件指令处理
    void transFileDeal (HEADER* pHeader);
    // 传输文件中指令处理
    void transFileIngDeal (HEADER* pHeader);
    // 发送文件失败指令处理
    void SendFileDeal (HEADER* pHeader);
    // 接收文件失败指令处理
    void RecvFileDeal (HEADER* pHeader);
    // yes指令处理
    void yesDeal (HEADER* pHeader);
    // no指令处理
    void noDeal (HEADER* pHeader);
    // 接收文件完成指令处理
    void transFileOKDeal (HEADER* pHeader);
    // 心跳指令处理
    void heartDeal (HEADER* pHeader);

    // 服务器心跳检查
    void heartBeatCheck (void);
    // 向服务器发送心跳
    void sendHeartBeat (void);

    pthread_t m_tid;  // 线程标识
};
#endif  // __COMMEND_H__
