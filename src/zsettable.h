#ifndef ZSETTABLE_H
#define ZSETTABLE_H

#include "table.h"
#include <vector>
#include <tr1/tuple>

class ZSetTable : public Table {

public:
	ZSetTable(std::string name);
	bool create();

	bool insert_row(ib_trx_t trx, uint64_t id, str val, double score);

	bool find_row(ib_trx_t trx, uint64_t id, str &val, ib_crsr_t &cursor);
	/*
	bool update_row(ib_trx_t trx, uint64_t id,
			int col_id, uint64_t val);

	bool delete_row(ib_trx_t trx, uint64_t id);
	*/

	//typedef std::tr1::tuple<uint64_t,str,uint64_t,uint64_t> RowData;

	//RowData read(ib_crsr_t cursor);
private:
	bool create_unique_index(ib_tbl_sch_t &schema);
};

class ZSetHeadTable : public Table {

public:
	ZSetHeadTable(std::string name);
	bool create();

	virtual bool load();

	bool zcard(str key, int &out);
	bool zadd(str key, double score, str val, int &out);
	/*
	bool lpush(str key, str val, int &out);
	bool rpush(str key, str val, int &out);
	bool lpop(str key, str &val);
	bool rpop(str key, str &val);

	void debug_dump(str key);
	bool lrange(str key, int start, int stop, std::vector<str> &out);
	bool llen(str key, int &out);
	*/

private:
	bool create_unique_index(ib_tbl_sch_t &schema);
	bool insert_row(ib_crsr_t cursor, str key, uint64_t &id, uint64_t card);
	bool update_row(ib_crsr_t cursor, ib_tpl_t row, uint64_t count);

	// row extraction
	typedef std::tr1::tuple<str,uint64_t,uint64_t> RowData;
	RowData read(ib_crsr_t cursor);
	static const int KEY = 0;
	static const int ID = 1;
	static const int COUNT = 2;
	ZSetTable m_zsets;
	bool
	get_cursor(str &key, ib_trx_t &trx,
			ib_crsr_t &cursor, ib_tpl_t &row);
};
#endif
