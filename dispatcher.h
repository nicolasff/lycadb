#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <tr1/functional>
#include <map>

class Store;
class Command;
class Result;


class Dispatcher {

public:
	Dispatcher(Store &s);
	Result* run(Command &cmd);


	typedef std::tr1::function<Result* (Command&)> Handler;

private:
	Store &m_store;

	std::map<std::string, Handler> m_functions;

	// implementations
	Result* get(Command &cmd);
	Result* set(Command &cmd);

};


#endif
