#ifndef STORE_H
#define STORE_H

#include <string>
#include <vector>
#include <haildb.h>

#include "kvtable.h"
#include "settable.h"

class Store {

public:
	Store(std::string db);

	bool startup();
	bool shutdown();

	bool install();

	// basic key management
	bool get(std::string key, std::string &val);
	bool set(std::string key, std::string val);

	// increment
	bool incr(std::string key, int by = 1);
	bool decr(std::string key, int by = 1);

	// sets
	bool sadd(std::string key, std::string val);
	bool sismember(std::string key, std::string val);
	bool smembers(std::string key, std::vector<std::string> &out);
	bool srem(std::string key, std::string val);

	// deletion
	bool del(std::string key);
	bool flushall();

private:
	bool createDb();
	bool createTable();

	std::string m_db;
	KVTable m_main;
	SetTable m_sets;
};

#endif
