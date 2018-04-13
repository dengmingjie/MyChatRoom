/* 定义异常类 */
#ifndef  __EXCEPT_H__
#define  __EXCEPT_H__
#include <string>
using namespace std;

/* 服务器异常 */
class ServerException: public exception
{
public:
	// 构造器
	ServerException (void): m_msg("服务器异常！") {}
    ServerException (string const& msg): m_msg("服务器异常：")
    {
		m_msg += msg;
		m_msg += "！";
	}
	// 析构器
	~ServerException (void) throw () {}
	// 获取异常信息
    char const* what (void) const throw ()
    {
		return m_msg.c_str ();
	}
private:
	string m_msg;  // 异常信息
};
/* 网络异常 */
class SocketException: public ServerException
{
public:
	SocketException (void):
        ServerException ("网络错误") {}
	SocketException (string const& msg):
        ServerException (msg) {}
};
/* 接收异常 */
class RecvException: public ServerException
{
public:
	RecvException (void):
		ServerException("接收错误") {}
	RecvException (string const& msg):
		ServerException (msg) {}
};
/* 发送异常 */
class SendException: public ServerException
{
public:
	SendException (void):
		ServerException ("发送错误") {}
	SendException (string const& msg):
		ServerException (msg) {}
};
/* 线程异常 */
class ThreadException: public ServerException
{
public:
	ThreadException (void):
		ServerException ("线程错误") {}
	ThreadException (string const& msg):
		ServerException (msg) {}
};
#endif  // __EXCEPT_H__
