/* 声明发送文件模块 */
#ifndef  __SENDFILE_H__
#define  __SENDFILE_H__
#include <pthread.h>
#include "except.h"
/* 发送文件模块 */
class CSendFile
{
public:
    // 启动发送文件模块
    void start (void) throw (ThreadException);
private:
    // 线程过程
    static void* run (void* arg);
    // 线程处理
    void* loopSend (void);

    pthread_t m_tid;  // 线程标识
};
#endif  // __SENDFILE_H__
