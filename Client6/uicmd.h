/* UICmd模块 */
#ifndef  __UICMD_H__
#define  __UICMD_H__
#include <pthread.h>
#include "except.h"
/* 声明UICmd模块类 */
class CUICmd
{
public:
    // 启动UICmd模块
    void start (void) throw (ThreadException);
private:
    // 线程过程
    static void* run (void* arg);
    // 线程处理
    void* loopUICmd (void);
    // 登录指令
    void logInCmd (void);
    // 私聊指令
    void pChatCmd (void);
    // 群聊指令
    void gChatCmd (void);
    // 传输文件指令
    void transFileCmd (void);
	// 接受传输文件指令
    void yesFileCmd (void);
	// 拒绝传输文件指令
    void noFileCmd (void);
    // 登出指令处理
    void logOutCmd (void);

    pthread_t m_tid;  // 线程标识
};
#endif  // __UICMD_H__
