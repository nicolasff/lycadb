#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstring> // size_t

typedef enum {
	CONSUME_OK,
	CONSUME_END,
	CONSUME_ERROR} parsing_err;

class NumParser {

public:
	NumParser();

	void reset();
	parsing_err consume(char c);

	operator int() const;

private:

	void consume_digit(char c);

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
	parsing_err consume(char c);

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
};

#endif
