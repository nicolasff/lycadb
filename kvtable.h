#ifndef KVTABLE_H
#define KVTABLE_H

#include "table.h"
#include "str.h"

class KVTable : public Table {

public:
	KVTable(std::string name);

	bool create();

	// basic key management
	bool get(str key, str *val);
	bool set(str key, str val);

	// increment
	bool incr(str key, int by = 1);
	bool decr(str key, int by = 1);
};

#endif
