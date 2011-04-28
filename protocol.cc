#include "protocol.h"
#include <cctype>
#include <iostream>

using namespace std;

NumParser::NumParser() {
	reset();
}

void
NumParser::reset() {
	state = ss_start;
	m_num = 0;
}

parsing_err
NumParser::consume(char c) {
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
NumParser::consume_digit(char c) {
	m_num *= 10;
	m_num += c - '0';
}

NumParser::operator int() const {
	return m_num;
}


////////////////////////////////////////////////////////////////////////////////

CommandParser::CommandParser() {
	reset();
}

void
CommandParser::reset() {
	state = ss_start;
	m_argc = 0;
}

parsing_err
CommandParser::consume(char c) {

	switch(state) {
		
		case ss_start:
			if(c == '*') {
				state = ss_star;
				m_num.reset();
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
					return CONSUME_END;
				} else {
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

