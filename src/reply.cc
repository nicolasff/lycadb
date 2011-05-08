#include "reply.h"
#include <sstream>
#include <iostream>

using namespace std;

Reply::~Reply() {
}

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

StringReply::~StringReply() {
	m_str.reset(); // discard my string
}

bool
StringReply::write(int fd) const {

	// format output
	stringstream ss;
	ss << '$' << m_str.size() << "\r\n";
	ss.write(m_str.c_str(), m_str.size());
	ss << "\r\n";

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

////////////////////////////////////////////////////////////////////////////////

ListReply::ListReply() {
}

ListReply::~ListReply() {
	vector<Reply*>::iterator ri;
	for(ri = m_elements.begin(); ri != m_elements.end(); ri++) {
		delete *ri;
	}
}

void
ListReply::add(Reply *r) {
	m_elements.push_back(r);
}

bool
ListReply::write(int fd) const {

	// write number of elements
	stringstream ss;
	ss << '*' << m_elements.size() << "\r\n";

	// send to fd
	string out = ss.str();
	int ret = ::write(fd, out.c_str(), out.size());

	// write all the elements one by one.
	vector<Reply*>::const_iterator ri;
	for(ri = m_elements.begin(); ri != m_elements.end(); ri++) {
		(*ri)->write(fd);
	}

	return (ret == (int)out.size());

}
