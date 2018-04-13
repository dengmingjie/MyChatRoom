// 定义TCP数据包
#ifndef  __PACKET_H__
#define  __PACKET_H__
enum commend{logIn, pChat, gChat, logOut};
typedef struct ST_BASE_HEADER {
    int version;
    int commID;
    int length;
    int userID;
} HEADER;
typedef struct ST_TCP_PACKET {
    HEADER header;
    char* userName[100];
    char* userPWD[100];
    int msg_len;
    char msg_buf[1024];
} PACKET;
#endif  // __PACKET_H__
