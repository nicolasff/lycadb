#include "table.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>

using namespace std;

Table::Table(string name)
	: m_name(name), m_tid(0) {

}

bool
Table::update_row(ib_crsr_t cursor, ib_tpl_t row, string val) {

	ib_err_t err;

	// compare to new value
	if(ib_col_get_len(row, 1) == val.size() &&
		::memcmp(ib_col_get_value(row, 1), val.c_str(), val.size()) == 0) {
		// same value!
		return true;
	}

	// update value
	ib_tpl_t new_row = ib_clust_read_tuple_create(cursor);
	err = ib_tuple_copy(new_row, row);
	err = ib_col_set_value(new_row, 1, val.c_str(), val.size());

	// insert new value in place.
	if((err = ib_cursor_update_row(cursor, row, new_row)) != DB_SUCCESS) {
		ib_tuple_delete(new_row);
		return false;
	}

	// delete row object
	ib_tuple_delete(new_row);
	return true;
}

bool
Table::insert_row(ib_crsr_t cursor, string key, string val) {

	ib_err_t err;

	// create row
	ib_tpl_t row = ib_clust_read_tuple_create(cursor);

	// set key and val columns
	err = ib_col_set_value(row, 0, key.c_str(), key.size());
	err = ib_col_set_value(row, 1, val.c_str(), val.size());

	// insert row
	bool ret = (ib_cursor_insert_row(cursor, row) == DB_SUCCESS);

	// delete row object
	ib_tuple_delete(row);

	return ret;
}

bool
Table::create_primary_index(ib_tbl_sch_t &schema) {

	ib_err_t err;

	// create primary key index.
	ib_idx_sch_t pk_index = NULL;
	if((err = ib_table_schema_add_index(schema, "PRIMARY_KEY", &pk_index)) != DB_SUCCESS) {
		return false;
	}

	// add `key` column to primary index.
	if((err = ib_index_schema_add_col(pk_index, "key", 0)) != DB_SUCCESS) {
		return false;
	}
	// set clustered index.
	if((err = ib_index_schema_set_clustered(pk_index)) != DB_SUCCESS) {
		return false;
	}

	return true;
}


void
Table::commit(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row) {

	ib_err_t err;

	if(cursor) err = ib_cursor_close(cursor);
	if(row) ib_tuple_delete(row);
	if(trx) err = ib_trx_commit(trx);
}

void
Table::rollback(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row) {
	ib_err_t err;

	if(cursor) err = ib_cursor_close(cursor);
	if(row) ib_tuple_delete(row);
	if(trx) err = ib_trx_commit(trx);
}

bool
Table::get_cursor(string &key, ib_trx_t &trx, ib_crsr_t &cursor, ib_tpl_t &row) {

	ib_err_t err;
	ib_tpl_t search_row;

	// begin transaction
	trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	cursor = 0;
	if((err = ib_cursor_open_table(m_name.c_str(), trx, &cursor)) != DB_SUCCESS) {
		cerr << ib_strerror(err) << endl;
	}

	// create seach tuple, handling update case.
	search_row = ib_clust_search_tuple_create(cursor);

	// set key column
	err = ib_col_set_value(search_row, 0, key.c_str(), key.size());

	// look for existing key
	int pos = -1;
	err = ib_cursor_moveto(cursor, search_row, IB_CUR_GE, &pos);

	// error handling
	if(err != DB_SUCCESS && err != DB_END_OF_INDEX) {
		rollback(trx, cursor, search_row);
		return false;
	}
	ib_tuple_delete(search_row); // no need for it anymore, we're positionned.

	if(pos == 0) { // return existing row.
		row = ib_clust_read_tuple_create(cursor);

		// read existing row
		if((err = ib_cursor_read_row(cursor, row)) != DB_SUCCESS) {
			rollback(trx, cursor, row);
			row = 0;
			return false;
		}
	} else {
		// no row found, but cursor in place.
		row = 0;
	}

	return true;
}

bool
Table::del(std::string key) {

	ib_trx_t trx = 0;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	// get table cursor
	if(get_cursor(key, trx, cursor, row) == false) {
		return false;
	}

	bool ret = false;
	if(row) { // found value
		ret = (ib_cursor_delete_row(cursor) == DB_SUCCESS);
	}

	// finish up.
	if(ret) {
		commit(trx, cursor, row);
	} else {
		rollback(trx, cursor, row);
	}

	return ret;
}

bool
Table::flushall() {
	ib_id_t tid = 0;
	return (ib_table_truncate(m_name.c_str(), &tid) == DB_SUCCESS);
}

bool
Table::install_schema(ib_tbl_sch_t &schema) {

	bool ret = true;

	ib_err_t err;

	ib_trx_t trx = ib_trx_begin(IB_TRX_SERIALIZABLE);
	if((err = ib_schema_lock_exclusive(trx)) != DB_SUCCESS) {
		ret = false;
	}
	if((err = ib_table_create(trx, schema, &m_tid)) != DB_SUCCESS) {
		ret = false;
	}
	if((err = ib_schema_unlock(trx)) != DB_SUCCESS) {
		ret = false;
	}
	if((err = ib_trx_commit(trx)) != DB_SUCCESS) {
		ret = false;
	}
	ib_table_schema_delete(schema);

	return ret;

}

