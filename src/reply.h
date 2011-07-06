#ifndef Reply_H
#define Reply_H

#include <string>
#include <vector>

#include "str.h"

class Reply {
public:
	virtual ~Reply();
	virtual bool write(int) const = 0;
};

////////////////////////////////////////////////////////////

class IntReply : public Reply {

public:
	IntReply(int val);
	virtual bool write(int) const;

private:
	int m_val;
};

////////////////////////////////////////////////////////////

class DoubleReply : public Reply {

public:
	DoubleReply(double val);
	virtual bool write(int) const;

private:
	double m_val;
};

////////////////////////////////////////////////////////////

class StringReply : public Reply {

public:
	StringReply(str s);
	~StringReply();
	virtual bool write(int) const;

private:
	str m_str;
};

////////////////////////////////////////////////////////////

class EmptyReply : public Reply {

public:
	EmptyReply();
	virtual bool write(int) const;

};

////////////////////////////////////////////////////////////

class ErrorReply : public Reply {

public:
	ErrorReply(std::string s);
	virtual bool write(int) const;

private:
	std::string m_str;
};

////////////////////////////////////////////////////////////

class SuccessReply : public Reply {

public:
	SuccessReply();
	virtual bool write(int) const;
};

////////////////////////////////////////////////////////////

class ListReply: public Reply {
public:
	ListReply();
	~ListReply();

	void add(Reply *r);
	virtual bool write(int) const;

private:
	std::vector<Reply*> m_elements;

};

#endif
