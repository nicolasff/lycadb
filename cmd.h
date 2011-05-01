#ifndef CMD_H
#define CMD_H

#include "str.h"
#include <vector>

class Command {

public:
	Command(int argc);
	void add(str s); 

	const str verb() const;

	size_t argc() const;
	str argv(int pos) const;

private:
	int m_argc;
	std::vector<str> m_args;

};

#endif
