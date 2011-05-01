#ifndef Reply_H
#define Reply_H

#include <string>

class Reply {
public:
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

class StringReply : public Reply {

public:
	StringReply(std::string &s);
	virtual bool write(int) const;

private:
	std::string m_str;
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

#endif
