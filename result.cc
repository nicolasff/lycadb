#include "result.h"
#include <sstream>
#include <iostream>

using namespace std;

///////////////////////////////////////////////////////////////////////////////

IntResult::IntResult(int val) : m_val(val) {
}

bool
IntResult::write(int fd) const {

	// format output
	stringstream ss;
	ss << ':' << m_val << "\r\n";

	// send to fd
	string out = ss.str();
	int ret = ::write(fd, out.c_str(), out.size());

	return (ret == (int)out.size());
}

////////////////////////////////////////////////////////////////////////////////

StringResult::StringResult(string &s) : m_str(s) {
}

bool
StringResult::write(int fd) const {

	// format output
	stringstream ss;
	ss << '$' << m_str.size() << "\r\n"
		<< m_str << "\r\n";

	// send to fd
	string out = ss.str();
	int ret = ::write(fd, out.c_str(), out.size());

	return (ret == (int)out.size());
}

////////////////////////////////////////////////////////////////////////////////

EmptyResult::EmptyResult() {
}

bool
EmptyResult::write(int fd) const {

	// send to fd
	char out[] = "$-1\r\n";
	int ret = ::write(fd, out, sizeof(out)-1);

	return (ret == (int)sizeof(out)-1);
}

////////////////////////////////////////////////////////////////////////////////

ErrorResult::ErrorResult(string s) : m_str(s) {
}

bool
ErrorResult::write(int fd) const {

	// format output
	stringstream ss;
	ss << "-ERR " << m_str << "\r\n";

	// send to fd
	string out = ss.str();
	int ret = ::write(fd, out.c_str(), out.size());

	return (ret == (int)out.size());
}
////////////////////////////////////////////////////////////////////////////////

SuccessResult::SuccessResult() {
}

bool
SuccessResult::write(int fd) const {

	// send to fd
	char out[] = "+OK\r\n";
	int ret = ::write(fd, out, sizeof(out)-1);

	return (ret == (int)sizeof(out)-1);

}
