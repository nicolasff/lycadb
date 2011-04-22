#ifndef SETTABLE_H
#define SETTABLE_H

#include "table.h"

class SetTable : public Table {

public:
	SetTable(std::string name);
	bool create();

	bool sadd(std::string key, std::string val);

private:
	bool create_unique_index(ib_tbl_sch_t &schema);

};

#endif
