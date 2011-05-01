#include "dispatcher.h"
#include "store.h"
#include "result.h"
#include "cmd.h"

#include <string>

using namespace std;
using namespace std::tr1::placeholders;

Dispatcher::Dispatcher(Store &s) : m_store(s) {

	m_functions["GET"] = tr1::bind(&Dispatcher::get, this, _1);
	m_functions["SET"] = tr1::bind(&Dispatcher::set, this, _1);
}


Result*
Dispatcher::run(Command &cmd) {

	string verb = cmd.verb();

	return m_functions[verb](cmd);
}

Result*
Dispatcher::get(Command &cmd) {

	string s("get-response");
	StringResult *sr = new StringResult(s);
	return sr;
}

Result*
Dispatcher::set(Command &cmd) {

	string s("set-response");
	StringResult *sr = new StringResult(s);
	return sr;
}
