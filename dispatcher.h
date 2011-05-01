#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <tr1/functional>
#include <map>

class Store;
class Command;
class Reply;


class Dispatcher {

public:
	Dispatcher(Store &s);
	Reply* run(Command &cmd);


	typedef std::tr1::function<Reply* (Command&)> Handler;

private:
	Store &m_store;

	std::map<std::string, Handler> m_functions;

	// implementations
	Reply* get(Command &cmd);
	Reply* set(Command &cmd);

};


#endif
