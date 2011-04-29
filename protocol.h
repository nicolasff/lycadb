#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstring> // size_t
#include <tr1/functional>

typedef enum {
	CONSUME_OK,
	CONSUME_END,
	CONSUME_ERROR} parsing_err;

class NumParser {

public:
	NumParser();

	void reset();
	parsing_err consume(const char c);

	operator int() const;

private:

	void consume_digit(const char c);

	enum {
		ss_start = 1,
		ss_num,
		ss_cr
	} state;

	int m_num;
};

class CommandParser {

public:
	CommandParser();

	void reset();
	parsing_err consume(const char c);
	void setArgcCb(std::tr1::function<void (int)> f);
	void setArgvCb(std::tr1::function<void (const char *, size_t)> f);

private:

	enum {
		ss_start = 1,
		ss_star,
		ss_argc,
		ss_dollar,
		ss_data,
		ss_cr,
		ss_lf
	} state;

	int m_argc;
	int m_curarg;
	char **m_argv;
	size_t *m_argvlen;
	char *m_arg;


	NumParser m_num;

	// callbacks
	std::tr1::function<void (int)> on_argc;
	std::tr1::function<void (const char *, size_t)> on_argv;
};

class Command;

class Parser {
public:
	Parser();
	~Parser();

	void setSuccessCb(std::tr1::function<void (Command*)> f);
	void setFailureCb(std::tr1::function<void (void)> f);
	void consume(const char *s, size_t sz);

private:
	CommandParser m_cp;
	Command *m_cmd;

	std::tr1::function<void (Command*)> on_success;
	std::tr1::function<void (void)> on_failure;

	void on_argc(int);
	void on_argv(const char *, size_t);
};

#endif
