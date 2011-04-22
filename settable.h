#ifndef SETTABLE_H
#define SETTABLE_H

#include "table.h"
#include <vector>

class SetTable : public Table {

public:
	SetTable(std::string name);
	bool create();

	bool sadd(std::string key, std::string val);
	bool sismember(std::string key, std::string val);

	bool smembers(std::string key, std::vector<std::string> &out);

private:
	bool create_unique_index(ib_tbl_sch_t &schema);

	bool
	get_cursor(std::string &key, std::string &val,
			ib_trx_t &trx, ib_crsr_t &cursor,
			ib_tpl_t &row);

};

#endif
