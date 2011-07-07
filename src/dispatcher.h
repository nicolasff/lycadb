#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "str.h"

#include <tr1/functional>
#include <map>

class Store;
class Command;
class Reply;

typedef std::tr1::function<Reply* (Command&)> CommandFun;

// a command handler checks for argc and runs the command.
class CommandHandler {
public:
	CommandHandler(CommandFun f = 0, size_t min = 1, size_t max = 1);
	Reply* operator()(Command &cmd);

private:
	CommandFun m_fun;
	size_t m_min, m_max;
};

/**
 * The Dispatcher object transforms a Command into a Reply.
 * It keeps a reference to the Store in order to do so.
 */
class Dispatcher {

public:
	Dispatcher(Store &s);
	Reply* run(Command &cmd);

private:
	Store &m_store;

	// function name to implementation
	std::map<str, CommandHandler> m_functions;

	// implementations
	Reply* get(Command &cmd);
	Reply* set(Command &cmd);
	Reply* del(Command &cmd);

	Reply* incr(Command &cmd);
	Reply* decr(Command &cmd);

	Reply* sadd(Command &cmd);
	Reply* smembers(Command &cmd);
	Reply* sismember(Command &cmd);
	Reply* srem(Command &cmd);

	Reply* lpush(Command &cmd);
	Reply* rpush(Command &cmd);
	Reply* llen(Command &cmd);
	Reply* lrange(Command &cmd);
	Reply* lpop(Command &cmd);
	Reply* rpop(Command &cmd);

	Reply* zadd(Command &cmd);
	Reply* zcard(Command &cmd);
	Reply* zrem(Command &cmd);
	Reply* zscore(Command &cmd);
	Reply* zcount(Command &cmd);
};


#endif
