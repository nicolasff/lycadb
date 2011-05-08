#ifndef LISTTABLE_H
#define LISTTABLE_H

#include "table.h"
#include <vector>

class ListTable : public Table {

public:
	ListTable(std::string name);
	bool create();

private:
	bool create_unique_index(ib_tbl_sch_t &schema);

	/*
	bool
	get_cursor(str &key, str &val,
			ib_trx_t &trx, ib_crsr_t &cursor,
			ib_tpl_t &row);
	*/
};

class ListHeadTable : public Table {

public:
	ListHeadTable(std::string name);
	bool create();

private:
	bool create_unique_index(ib_tbl_sch_t &schema);

	/*
	bool
	get_cursor(str &key, str &val,
			ib_trx_t &trx, ib_crsr_t &cursor,
			ib_tpl_t &row);
	*/
};
#endif
