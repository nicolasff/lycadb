#ifndef CMD_H
#define CMD_H

#include <string>
#include <vector>

class Command {

public:
	Command(int argc);
	void add(std::string s); 

	const std::string verb() const;

	size_t argc() const;
	std::string argv(int pos) const;

private:
	int m_argc;
	std::vector<std::string> m_args;

};

#endif
