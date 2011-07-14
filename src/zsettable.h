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
	bool zcount(ib_trx_t trx, uint64_t id, double min, double max, int &out);
	/*
	bool update_row(ib_trx_t trx, uint64_t id,
			int col_id, uint64_t val);

	*/
	bool delete_row(ib_crsr_t cursor);

	typedef std::tr1::tuple<uint64_t,str,double> RowData;
	typedef std::tr1::tuple<uint64_t,double,str> IdxRowData;

	RowData read(ib_crsr_t cursor);
	IdxRowData read_idx(ib_crsr_t cursor);

	static const int ID = 0;
	static const int VAL = 1;
	static const int SCORE = 2;

	static const int IDX_ID = 0;
	static const int IDX_SCORE = 1;
	static const int IDX_VAL = 2;

private:
	bool create_unique_index(ib_tbl_sch_t &schema);
	bool create_score_index(ib_tbl_sch_t &schema);
    void pad_score(double score, std::string &ret);
};

class ZSetHeadTable : public Table {

public:
	ZSetHeadTable(std::string name);
	bool create();

	virtual bool load();

	bool zcard(str key, int &out);
	bool zadd(str key, double score, str val, int &out);
	bool zrem(str key, str val, int &out);
	bool zscore(str key, str val, double &out, bool &found);
	bool zcount(str key, double min, double max, int &out);

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
