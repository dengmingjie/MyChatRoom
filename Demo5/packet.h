/* 定义TCP数据包 */
#ifndef  __PACKET_H__
#define  __PACKET_H__
enum comm{logIn, pChat, gChat, logOut, logIn_SUCCESS, pChat_FAILURE, heartBeat};
#pragma pack(1) /* 按１个字节补齐 */
typedef struct ST_BASE_HEADER
{
    int version;
    int commID;
    int length;
    int userID;
} HEADER;
typedef struct ST_TCP_PACKET
{
	int to_userID;
    char userName[20];
    char userPWD[20];
    char msg_buf[1024];
} PACKET;
#pragma pack()  /* 取消字节补齐 */
#endif  // __PACKET_H__
