#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <haildb.h>

class Table {

public:
	Table(std::string name);

	virtual bool create() = 0;

	// deletion
	virtual bool del(std::string key);
	bool flushall();

protected:

	bool
	get_cursor(std::string &key, ib_trx_t &trx,
			ib_crsr_t &cursor, ib_tpl_t &row);

	bool
	update_row(ib_crsr_t cursor, ib_tpl_t row, std::string val);

	bool
	insert_row(ib_crsr_t cursor, std::string key, std::string val);

	void
	commit(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row);

	void
	rollback(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row);

	bool
	install_schema(ib_tbl_sch_t &schema);

	std::string m_name;
	ib_id_t m_tid; // table id
};

#endif
