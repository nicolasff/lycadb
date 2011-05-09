#ifndef LISTTABLE_H
#define LISTTABLE_H

#include "table.h"
#include <vector>
#include <tr1/tuple>

class ListTable : public Table {

public:
	ListTable(std::string name);
	bool create();

	uint64_t insert_row(ib_trx_t trx, str val,
			uint64_t prev, uint64_t next);

	bool find_row(ib_trx_t trx, uint64_t id, ib_crsr_t &cursor);
	bool update_row(ib_trx_t trx, uint64_t id,
			int col_id, uint64_t val);

	bool delete_row(ib_trx_t trx, uint64_t id);

	static const int ID = 0;
	static const int VAL = 1;
	static const int PREV = 2;
	static const int NEXT = 3;

	typedef std::tr1::tuple<uint64_t,str,uint64_t,uint64_t> RowData;

	RowData read(ib_crsr_t cursor);
private:
	bool create_unique_index(ib_tbl_sch_t &schema);
};

class ListHeadTable : public Table {

public:
	ListHeadTable(std::string name);
	bool create();

	virtual bool load();

	bool lpush(str key, str val, int &out);
	bool rpush(str key, str val, int &out);
	bool lpop(str key, str &val);
	bool rpop(str key, str &val);

	void debug_dump(str key);
	bool lrange(str key, int start, int stop, std::vector<str> &out);
	bool llen(str key, int &out);

private:
	bool create_unique_index(ib_tbl_sch_t &schema);
	bool insert_row(ib_crsr_t cursor, str key,
			uint64_t head, uint64_t tail,
			uint64_t count);
	bool update_row(ib_crsr_t cursor, ib_tpl_t row,
			uint64_t head, uint64_t tail,
			uint64_t count);

	// row extraction
	typedef std::tr1::tuple<str,uint64_t,uint64_t,uint64_t> RowData;
	RowData read(ib_crsr_t cursor);
	static const int KEY = 0;
	static const int HEAD = 1;
	static const int TAIL = 2;
	static const int COUNT = 3;

	ListTable m_lists;

	bool
	get_cursor(str &key, ib_trx_t &trx,
			ib_crsr_t &cursor, ib_tpl_t &row);
};
#endif
