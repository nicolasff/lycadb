#include "cmd.h"

using namespace std;

Command::Command(int argc) :
	m_argc(argc) {
	
	m_args.reserve(argc);

}

Command::~Command() {
	for(unsigned int i = 0; i < m_args.size(); ++i) {
		m_args[i].reset();
	}
}

void
Command::add(str s) {
	m_args.push_back(s);
}

const str
Command:: verb() const {
	return m_args[0];
}

size_t
Command::argc() const {
	return m_args.size()-1;
}

str
Command::argv(int pos) const {
	return m_args[pos];
}
