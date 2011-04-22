#ifndef STORE_H
#define STORE_H

#include <string>
#include <haildb.h>

class Store {

public:
	Store(std::string db, std::string table);
	bool startup();
	bool shutdown();

	bool install();

	bool get(std::string key, std::string &val);
	bool set(std::string key, std::string val);

	bool incr(std::string key, int by = 1);
	bool decr(std::string key, int by = 1);

	// deletion
	bool del(std::string key);
	bool flushall();

private:
	bool createDb();
	bool createTable();

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

	std::string m_db, m_table;
};

#endif
