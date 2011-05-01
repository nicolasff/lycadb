#include "reply.h"
#include <sstream>
#include <iostream>

using namespace std;

///////////////////////////////////////////////////////////////////////////////

IntReply::IntReply(int val) : m_val(val) {
}

bool
IntReply::write(int fd) const {

	// format output
	stringstream ss;
	ss << ':' << m_val << "\r\n";

	// send to fd
	string out = ss.str();
	int ret = ::write(fd, out.c_str(), out.size());

	return (ret == (int)out.size());
}

////////////////////////////////////////////////////////////////////////////////

StringReply::StringReply(str s) : m_str(s) {
}

bool
StringReply::write(int fd) const {

	// format output
	stringstream ss;
	ss << '$' << m_str.size() << "\r\n"
		<< m_str.c_str() << "\r\n";

	// send to fd
	string out = ss.str();
	int ret = ::write(fd, out.c_str(), out.size());

	return (ret == (int)out.size());
}

////////////////////////////////////////////////////////////////////////////////

EmptyReply::EmptyReply() {
}

bool
EmptyReply::write(int fd) const {

	// send to fd
	char out[] = "$-1\r\n";
	int ret = ::write(fd, out, sizeof(out)-1);

	return (ret == (int)sizeof(out)-1);
}

////////////////////////////////////////////////////////////////////////////////

ErrorReply::ErrorReply(string s) : m_str(s) {
}

bool
ErrorReply::write(int fd) const {

	// format output
	stringstream ss;
	ss << "-ERR " << m_str << "\r\n";

	// send to fd
	string out = ss.str();
	int ret = ::write(fd, out.c_str(), out.size());

	return (ret == (int)out.size());
}
////////////////////////////////////////////////////////////////////////////////

SuccessReply::SuccessReply() {
}

bool
SuccessReply::write(int fd) const {

	// send to fd
	char out[] = "+OK\r\n";
	int ret = ::write(fd, out, sizeof(out)-1);

	return (ret == (int)sizeof(out)-1);

}
