#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <haildb.h>

#include "str.h"

class Table {

public:
	Table(std::string name);

	virtual bool create() = 0;
	virtual bool load();

	// deletion
	virtual bool del(str key);
	bool flushall();

protected:

	bool
	get_cursor(str key, ib_trx_t &trx,
			ib_crsr_t &cursor, ib_tpl_t &row);

	bool update_row(ib_crsr_t cursor, ib_tpl_t row, str val);

	bool insert_row(ib_crsr_t cursor, str key, str val);

	void commit(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row);

	void rollback(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row);

	bool create_primary_index(ib_tbl_sch_t &schema);

	bool install_schema(ib_tbl_sch_t &schema);

	std::string m_name;
	ib_id_t m_tid; // table id
};

#endif
