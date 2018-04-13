/* 定义TCP数据包 */
#ifndef  __PACKET_H__
#define  __PACKET_H__
enum comm {logIn, pChat, gChat, transFile, logOut, logIn_SUCCESS, pChat_FAILURE, transFile_ING, transFile_SENDFAIL, transFile_RECVFAIL, transFile_YES, transFile_NO, transFile_OK, heartBeat};
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
    HEADER header;
    int to_userID;
    int packetNum;
    char data[1000];
} PACKET;
#pragma pack()  /* 取消字节补齐 */
#endif  // __PACKET_H__
