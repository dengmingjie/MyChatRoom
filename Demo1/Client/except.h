// 定义异常类
#ifndef  __EXCEPT_H__
#define  __EXCEPT_H__
#include <string>
using namespace std;

// 客户端异常
class ClientException: public exception {
public:
	ClientException (void): 
		m_msg("客户端异常！") {}
	ClientException (string const& msg): 
		m_msg("客户端异常：") {
		m_msg += msg;
		m_msg += "！";
	}
	~ClientException (void) throw () {}
	// 获取异常信息
	char const* what (void) const throw () {
		return m_msg.c_str ();
	}
private:
	string m_msg;  // 异常信息
};
// 网络异常
class SocketException: public ClientException {
public:
	SocketException (void):
		ClientException ("网络错误") {}
	SocketException (string const& msg):
		ClientException (msg) {}
};
// 接收异常
class RecvException: public ClientException {
public:
	RecvException (void):
		ClientException ("接收错误") {}
	RecvException (string const& msg):
		ClientException (msg) {}
};
// 发送异常
class SendException: public ClientException {
public:
	SendException (void):
		ClientException ("发送错误") {}
	SendException (string const& msg):
		ClientException (msg) {}
};
#endif  // __EXCEPT_H__
