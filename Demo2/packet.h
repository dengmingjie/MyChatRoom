// 定义TCP数据包
#ifndef  __PACKET_H__
#define  __PACKET_H__
typedef struct TAG_TCP_PACKET {
	int msg_len;
	char msg_buf[1024];
} PACKET;
#endif  // __PACKET_H__
