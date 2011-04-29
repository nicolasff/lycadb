#include "cmd.h"

using namespace std;

Command::Command(int argc) :
	m_argc(argc) {
	
	m_args.reserve(argc);

}

void
Command::add(string s) {
	m_args.push_back(s);
}

const string
Command:: verb() const {
	return m_args[0];
}

