#include "dispatcher.h"
#include "store.h"
#include "reply.h"
#include "cmd.h"

#include <string>

using namespace std;
using namespace std::tr1::placeholders;

Dispatcher::Dispatcher(Store &s) : m_store(s) {

	m_functions["GET"] = tr1::bind(&Dispatcher::get, this, _1);
	m_functions["SET"] = tr1::bind(&Dispatcher::set, this, _1);
}


Reply*
Dispatcher::run(Command &cmd) {

	string verb = cmd.verb();

	return m_functions[verb](cmd);
}

Reply*
Dispatcher::get(Command &cmd) {

	if(cmd.argc() != 1) {
		return new ErrorReply("wrong number of arguments");
	}

	string key = cmd.argv(1), val;
	if(m_store.get(key, val)) {
		return new StringReply(val);
	} else {
		return new EmptyReply();
	}
}

Reply*
Dispatcher::set(Command &cmd) {

	if(cmd.argc() != 2) {
		return new ErrorReply("wrong number of arguments");
	}

	string key = cmd.argv(1), val = cmd.argv(2);
	if(m_store.set(key, val)) {
		return new SuccessReply();
	} else {
		return new ErrorReply("unknown error");
	}
}
