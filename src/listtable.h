#ifndef LISTTABLE_H
#define LISTTABLE_H

#include "table.h"
#include <vector>

class ListTable : public Table {

public:
	ListTable(std::string name);
	bool create();

	uint64_t insert_row(ib_trx_t trx, str val,
			uint64_t prev, uint64_t next);

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

	bool lpush(str key, str val, int &out);

private:
	bool create_unique_index(ib_tbl_sch_t &schema);
	bool insert_row(ib_crsr_t cursor, str key,
			uint64_t head, uint64_t tail,
			uint64_t count);
	bool update_row(ib_crsr_t cursor, ib_tpl_t row,
			uint64_t head, uint64_t tail,
			uint64_t count);

	ListTable m_lists;

	/*
	bool
	get_cursor(str &key, str &val,
			ib_trx_t &trx, ib_crsr_t &cursor,
			ib_tpl_t &row);
	*/
};
#endif
