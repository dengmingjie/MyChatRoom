/* 实现接收缓存区类 */
#ifndef  __RECVBUF_H__
#define  __RECVBUF_H__
#include <string.h>
#include "packet.h"
/* 接收缓存区类 */
class CRecvBuf
{
public:
	// 构造器
	CRecvBuf (void)
	{
 		m_curSize = 0;
 		m_allSize = 0;
		m_isHeaderFull = false;
    	memset (m_recvBuf, 0, sizeof (m_recvBuf));
	}

    HEADER header;        // 包头
    char m_recvBuf[1500]; // 接收缓存区
    int m_curSize;        // 当前接收长度
    int m_allSize;        // 数据报总长度
    bool m_isHeaderFull;  // 是否接收完包头
};
#endif  // __RECVBUF_H__
