#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "str.h"

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

	std::map<str, Handler> m_functions;

	// implementations
	Reply* get(Command &cmd);
	Reply* set(Command &cmd);
	Reply* del(Command &cmd);

	//Reply* incr(Command &cmd);
	//Reply* decr(Command &cmd);

	Reply* sadd(Command &cmd);
	Reply* smembers(Command &cmd);
	Reply* sismember(Command &cmd);
	Reply* srem(Command &cmd);

};


#endif
