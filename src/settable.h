#ifndef SETTABLE_H
#define SETTABLE_H

#include "table.h"
#include <vector>

class SetTable : public Table {

public:
	SetTable(std::string name);
	bool create();

	bool sadd(str key, str val);
	bool sismember(str key, str val);
	bool smembers(str key, std::vector<str> &out);
	bool srem(str key, str val);

private:
	bool create_unique_index(ib_tbl_sch_t &schema);

	bool
	get_cursor(str &key, str &val,
			ib_trx_t &trx, ib_crsr_t &cursor,
			ib_tpl_t &row);

};

#endif
