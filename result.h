#ifndef RESULT_H
#define RESULT_H

#include <string>

class Result {
public:
	virtual bool write(int) const = 0;
};

////////////////////////////////////////////////////////////

class IntResult : public Result {

public:
	IntResult(int val);
	virtual bool write(int) const;

private:
	int m_val;
};

////////////////////////////////////////////////////////////

class StringResult : public Result {

public:
	StringResult(std::string &s);
	virtual bool write(int) const;

private:
	std::string m_str;
};

////////////////////////////////////////////////////////////

#endif
