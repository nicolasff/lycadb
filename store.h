#ifndef STORE_H
#define STORE_H

#include <string>
#include <haildb.h>

#include "kvtable.h"

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

	bool del(std::string key);
	bool flushall();

private:
	bool createDb();
	bool createTable();

	std::string m_db;
	KVTable m_main;
};

#endif
