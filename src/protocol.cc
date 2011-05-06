#include "protocol.h"
#include "cmd.h"

#include <cctype>
#include <functional>

using namespace std;
using namespace std::tr1::placeholders;

/**
 * NumParser state machine
 *
 *                          [0..9]--+
 *                            |     |
 *                            |    /
 *                            v   /
 *  ss_start --> [1..9] --> ss_num --> CR --> ss_cr
 *                             \               /
 *                              LF            /
 *                               \           /
 *       			 -->  ø  <--
 *
 */

NumParser::NumParser() {
	reset();
}

void
NumParser::reset() {
	state = ss_start;
	m_num = 0;
}

parsing_err
NumParser::consume(const char c) {
	switch(state) {
		case ss_start:
			if(isdigit(c) && c != '0') {
				consume_digit(c);
				state = ss_num;
				return CONSUME_OK;
			}
			break;

		case ss_num:
			if(isdigit(c)) {
				consume_digit(c);
				state = ss_num;
				return CONSUME_OK;
			} else if(c == '\r') {
				state = ss_cr;
				return CONSUME_OK;
			} else if(c == '\n') {
				state = ss_start;
				return CONSUME_OK;
			}
			break;

		case ss_cr:
			if(c == '\n') {
				state = ss_start;
				return CONSUME_END;
			}
			break;
	}

	return CONSUME_ERROR;
}

void
NumParser::consume_digit(const char c) {
	m_num *= 10;
	m_num += c - '0';
}

NumParser::operator int() const {
	return m_num;
}


////////////////////////////////////////////////////////////////////////////////

CommandParser::CommandParser(Parser &parent)
	: state(ss_start)
	, m_argc(0)
	, m_argv(0)
	, m_argvlen(0)
	, m_parent(parent) {
}

void
CommandParser::reset() {
	state = ss_start;

	for(int i = 0; i < m_argc; ++i) {
		delete[](m_argv[i]);
	}

	delete[](m_argv);
	delete[](m_argvlen);

	m_argv = 0;
	m_argvlen = 0;
	m_argc = 0;
}

parsing_err
CommandParser::consume(const char c) {

	switch(state) {

		case ss_start:
			if(c == '*') { // reading chunks.
				state = ss_star;
				m_num.reset();
				return CONSUME_OK;
			} else if(isalpha(c)) { // first char is a letter
				m_parent.on_argc(0);
				m_word.push_back(c);
				state = ss_letter;
				return CONSUME_OK;
			}
			break;

		case ss_letter:
			if(c == '\r') {	// end of line approaching
				return CONSUME_OK;
			} else if(c == '\n') {	// end of the line.
				if(!m_word.empty()) {	// process last word.
					m_parent.on_argv(&m_word[0], m_word.size());
					m_word.clear();
				}
				state = ss_lf;
				return CONSUME_END;
			} else if(isspace(c)) { // end of word
				m_parent.on_argv(&m_word[0], m_word.size());
				m_word.clear();
				return CONSUME_OK;
			} else {	// add to current word.
				m_word.push_back(c);
				return CONSUME_OK;
			}
			break;

		case ss_star:
			switch(m_num.consume(c)) {
				case CONSUME_END:
					m_argc = m_num;
					m_argv = new char*[m_argc];
					m_argvlen = new size_t[m_argc];
					m_curarg = 0;
					state = ss_argc;
					m_parent.on_argc(m_argc);
					return CONSUME_OK;

				case CONSUME_OK:
					return CONSUME_OK;

				case CONSUME_ERROR:
					return CONSUME_ERROR;
			}

		case ss_argc:
			if(c == '$') {
				state = ss_dollar;
				m_num.reset();
				return CONSUME_OK;
			}
			return CONSUME_ERROR;

		case ss_dollar:
			switch(m_num.consume(c)) {
				case CONSUME_END:
					m_argvlen[m_curarg] = m_num;
					m_arg = m_argv[m_curarg] = new char[m_num];
					m_num.reset();
					state = ss_data;
					return CONSUME_OK;

				case CONSUME_OK:
					return CONSUME_OK;

				case CONSUME_ERROR:
					return CONSUME_ERROR;
			}

		case ss_data:
			if(m_arg == m_argv[m_curarg] + m_argvlen[m_curarg]) { // just one after the data
				if(c == '\r') {
					state = ss_cr;
					return CONSUME_OK;
				} else if(c == '\n') {
					state = ss_lf;
					return CONSUME_END;
				} else {
					return CONSUME_ERROR;
				}
			} else {
				*(m_arg++) = c;
				return CONSUME_OK;
			}

			break;

		case ss_cr:
			if(c == '\n') {
				state = ss_lf;
				if(++m_curarg == m_argc) {
					// last callback
					m_parent.on_argv(m_argv[m_curarg-1], m_argvlen[m_curarg-1]);
					return CONSUME_END;
				} else {
					// callback with complete argument
					m_parent.on_argv(m_argv[m_curarg-1], m_argvlen[m_curarg-1]);
					state = ss_argc;
					return CONSUME_OK;
				}
			}
			break;

		case ss_lf:
			return CONSUME_END;

		default:
			break;

	}

	return CONSUME_ERROR;
}

////////////////////////////////////////////////////////////////////////////////

Parser::Parser(): m_cp(*this) {
}

Parser::~Parser() {
}

void
Parser::consume(const char *s, size_t sz) {

	for(const char *p = s; p != s+sz; ++p) {
		switch(m_cp.consume(*p)) {
			case CONSUME_OK:
				continue;

			case CONSUME_ERROR:
				// TODO: cleanup
				if(on_failure) on_failure();
				m_cp.reset();
				break;

			case CONSUME_END:
				// TODO: cleanup
				if(on_success) on_success(m_cmd);
				m_cp.reset();
				break;
		}
	}
}

void
Parser::setSuccessCb(tr1::function<void (Command*)> f) {
	on_success = f;
}

void
Parser::setFailureCb(tr1::function<void (void)> f) {
	on_failure = f;
}

void
Parser::on_argc(int n) {
	// create a new command, allocating space if possible.
	m_cmd = new Command(n);
}

void
Parser::on_argv(const char *s, size_t sz) {
	// add argument, duplicating the buffer.
	m_cmd->add(str(s, sz, 1));
}

