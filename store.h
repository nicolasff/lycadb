#ifndef STORE_H
#define STORE_H

#include <string>
#include <haildb.h>

#include "table.h"

class Store {

public:
	Store(std::string db);
	bool startup();
	bool shutdown();

	bool install();

private:
	bool createDb();
	bool createTable();

	std::string m_db;
	Table m_table;
};

#endif
