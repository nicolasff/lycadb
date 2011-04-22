#ifndef KVTABLE_H
#define KVTABLE_H

#include "table.h"

class KVTable : public Table {

public:
	KVTable(std::string name);

	bool create();

	// basic key management
	bool get(std::string key, std::string &val);
	bool set(std::string key, std::string val);

	// increment
	bool incr(std::string key, int by = 1);
	bool decr(std::string key, int by = 1);
};

#endif
