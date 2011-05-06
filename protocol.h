#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstring> // size_t
#include <vector>
#include <tr1/functional>

typedef enum {
	CONSUME_OK,
	CONSUME_END,
	CONSUME_ERROR} parsing_err;

// Parse a number, such as the number of parameters
// or the length of a string.
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

class Parser;
// Parse a whole command, either in line mode
// or in multi-chunk mode.
class CommandParser {

public:
	CommandParser(Parser &parent);

	void reset();
	parsing_err consume(const char c);

private:

	enum {
		ss_start = 1,

		// simple line parser
		ss_letter,
		ss_space,

		// chunked protocol parser
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

	// current word in line-reading mode.
	std::vector<char> m_word;

	NumParser m_num;	// used to parse numbers.
	Parser &m_parent;	// used to send callbacks.
};

class Command;

class Parser {
public:
	Parser();
	~Parser();

	// register callbacks
	void setSuccessCb(std::tr1::function<void (Command*)> f);
	void setFailureCb(std::tr1::function<void (void)> f);
	void consume(const char *s, size_t sz);

private:
	CommandParser m_cp;
	Command *m_cmd;

	// callbacks
	std::tr1::function<void (Command*)> on_success;
	std::tr1::function<void (void)> on_failure;

	// called from CommandParser
	void on_argc(int);
	void on_argv(const char *, size_t);
	friend class CommandParser;
};

#endif
