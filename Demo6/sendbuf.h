/* 实现发送缓存区类 */
#ifndef  __SENDBUF_H__
#define  __SENDBUF_H__
#include <string.h>
#include "packet.h"
/* 发送缓存区类 */
class CSendBuf
{
public:
	// 构造器
	CSendBuf (void)
	{
    	m_curSize = 0;
    	m_allSize = 0;
    	memset (m_sendBuf, 0, sizeof (m_sendBuf));
	}

    int m_curSize;  // 当前发送长度
    int m_allSize;  // 数据包总长度
    char m_sendBuf[1500]; // 发送缓存区
};
#endif  // __SENDBUF_H__
