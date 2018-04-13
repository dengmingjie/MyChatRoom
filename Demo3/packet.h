/* 定义TCP数据包 */
#ifndef  __PACKET_H__
#define  __PACKET_H__
enum comm{logIn, pChat, gChat, logOut, LogIn_SUCCESS};
typedef struct ST_BASE_HEADER
{
    int version;
    int commID;
    int length;
    int userID;
	int to_userID;
} HEADER;
typedef struct ST_TCP_PACKET
{
    HEADER header;
    char userName[20];
    char userPWD[20];
    char msg_buf[1024];
} PACKET;
#endif  // __PACKET_H__
