#include "dispatcher.h"
#include "store.h"
#include "reply.h"
#include "cmd.h"

#include <string>
#include <cstdlib>

using namespace std;
using namespace std::tr1::placeholders;

Dispatcher::Dispatcher(Store &s) : m_store(s) {

	// bind internal, private methods to keywords.

	m_functions["GET"] = tr1::bind(&Dispatcher::get, this, _1);
	m_functions["SET"] = tr1::bind(&Dispatcher::set, this, _1);
	m_functions["DEL"] = tr1::bind(&Dispatcher::del, this, _1);

	m_functions["INCR"] = tr1::bind(&Dispatcher::incr, this, _1);
	m_functions["DECR"] = tr1::bind(&Dispatcher::decr, this, _1);

	m_functions["SADD"] = tr1::bind(&Dispatcher::sadd, this, _1);
	m_functions["SMEMBERS"] = tr1::bind(&Dispatcher::smembers, this, _1);
	m_functions["SISMEMBER"] = tr1::bind(&Dispatcher::sismember, this, _1);
	m_functions["SREM"] = tr1::bind(&Dispatcher::srem, this, _1);
}


Reply*
Dispatcher::run(Command &cmd) {

	// look for command
	map<str, Handler>::iterator fun = m_functions.find(cmd.verb());

	if(fun == m_functions.end()) {	// not found in command table.
		return new ErrorReply("Unknown command");
	} else {
		// execute
		return fun->second(cmd);
	}
}

////////////////////////////////////////////////////////////////////////////////

Reply*
Dispatcher::get(Command &cmd) {

	if(cmd.argc() != 1) {
		return new ErrorReply("wrong number of arguments");
	}

	str key(cmd.argv(1)), val;
	if(m_store.get(key, &val)) {
		return new StringReply(val); // the reply object owns `val'.
	} else {
		return new EmptyReply();
	}
}


Reply*
Dispatcher::set(Command &cmd) {

	if(cmd.argc() != 2) {
		return new ErrorReply("wrong number of arguments");
	}

	if(m_store.set(cmd.argv(1), cmd.argv(2))) {
		return new SuccessReply();
	} else {
		return new ErrorReply("unknown error");
	}
}

Reply*
Dispatcher::del(Command &cmd) {

	if(cmd.argc() != 1) {
		return new ErrorReply("wrong number of arguments");
	}

	if(m_store.del(cmd.argv(1))) {
		return new IntReply(1);
	} else {
		return new IntReply(0);
	}
}

Reply*
Dispatcher::incr(Command &cmd) {
	if(cmd.argc() != 1 && cmd.argc() != 2) {
		return new ErrorReply("wrong number of arguments");
	}

	int out;
	int by = cmd.argc() == 1 ? 1 : ::atol(cmd.argv(2).c_str());
	if(m_store.incr(cmd.argv(1), by, out)) {
		return new IntReply(out);
	} else {
		return new ErrorReply("unknown error");
	}
}

Reply*
Dispatcher::decr(Command &cmd) {
	if(cmd.argc() != 1 && cmd.argc() != 2) {
		return new ErrorReply("wrong number of arguments");
	}

	int out;
	int by = cmd.argc() == 1 ? 1 : ::atol(cmd.argv(2).c_str());
	if(m_store.decr(cmd.argv(1), by, out)) {
		return new IntReply(out);
	} else {
		return new ErrorReply("unknown error");
	}
}

Reply*
Dispatcher::sadd(Command &cmd) {

	if(cmd.argc() != 2) {
		return new ErrorReply("wrong number of arguments");
	}

	if(m_store.sadd(cmd.argv(1), cmd.argv(2))) {
		return new IntReply(1);
	} else {
		return new IntReply(0);
	}
}

Reply*
Dispatcher::smembers(Command &cmd) {

	if(cmd.argc() != 1) {
		return new ErrorReply("wrong number of arguments");
	}

	vector<str> out;
	if(m_store.smembers(cmd.argv(1), out)) {
		ListReply *r = new ListReply;
		for(vector<str>::iterator i = out.begin(); i != out.end(); i++) {
			r->add(new StringReply(*i));
		}
		return r;
	} else {
		return new ErrorReply("unknown error");
	}
}

Reply*
Dispatcher::sismember(Command &cmd) {

	if(cmd.argc() != 2) {
		return new ErrorReply("wrong number of arguments");
	}

	if(m_store.sismember(cmd.argv(1), cmd.argv(2))) {
		return new IntReply(1);
	} else {
		return new IntReply(0);
	}
}

Reply*
Dispatcher::srem(Command &cmd) {

	if(cmd.argc() != 2) {
		return new ErrorReply("wrong number of arguments");
	}

	if(m_store.srem(cmd.argv(1), cmd.argv(2))) {
		return new IntReply(1);
	} else {
		return new IntReply(0);
	}
}
