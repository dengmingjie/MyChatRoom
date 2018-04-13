/* 声明互斥锁类 */
#ifndef CMUTEX_H
#define CMUTEX_H
#include <pthread.h>
/* 互斥锁类 */
class CMutex
{
public:
    // 构造器
    CMutex (pthread_mutex_t& mutex);
    // 析构器
    ~CMutex (void);
private:
    pthread_mutex_t m_mutex;  // 互斥锁
};
#endif // CMUTEX_H
