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

