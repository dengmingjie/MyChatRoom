/* 实现互斥锁类 */
#include "mutex.h"
/* 构造器 */
CMutex::CMutex (pthread_mutex_t& mutex): m_mutex(mutex)
{
    pthread_mutex_lock (&m_mutex);
}
/* 析构器 */
CMutex::~CMutex (void)
{
    pthread_mutex_unlock (&m_mutex);
}
