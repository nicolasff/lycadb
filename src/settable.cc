#include "settable.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>

using namespace std;

SetTable::SetTable(string name) : Table(name) {

}

bool
SetTable::create() {

	ib_err_t err;
	ib_tbl_sch_t schema = NULL;

	// create schema
	if((err = ib_table_schema_create(m_name.c_str(), &schema, IB_TBL_COMPACT, 0)) != DB_SUCCESS) {
		return false;
	}

	// add `key` column as VARCHAR(64)
	if((err = ib_table_schema_add_col(schema, "key", IB_VARCHAR, IB_COL_NOT_NULL, 0, 64)) != DB_SUCCESS) {
		return false;
	}

	// add `val` column as BLOB
	if((err = ib_table_schema_add_col(schema, "val", IB_BLOB, IB_COL_NOT_NULL, 0, 0)) != DB_SUCCESS) {
		return false;
	}

	if(!create_unique_index(schema)) {
		return false;
	}

	// begin transaction
	return install_schema(schema);
}

bool
SetTable::create_unique_index(ib_tbl_sch_t &schema) {

	ib_err_t err;

	// create primary key index.
	ib_idx_sch_t unique_index = NULL;
	if((err = ib_table_schema_add_index(schema, "UNIQUE_KEY", &unique_index)) != DB_SUCCESS) {
		return false;
	}

	// add `key` column to index.
	if((err = ib_index_schema_add_col(unique_index, "key", 0)) != DB_SUCCESS) {
		return false;
	}

	// add `val` column to index.
	if((err = ib_index_schema_add_col(unique_index, "val", 0)) != DB_SUCCESS) {
		return false;
	}

	// set unique index.
	if((err = ib_index_schema_set_clustered(unique_index)) != DB_SUCCESS) {
		return false;
	}

	return true;
}

bool
SetTable::sadd(str key, str val) {

	bool ret = false;

	ib_trx_t trx = 0;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	// get table cursor
	if(get_cursor(key, val, trx, cursor, row) == false) {
		return false;
	}

	if(row) { // existing value
		rollback(trx, cursor, row);
		return false;
	} else { // insert value.
		ret = insert_row(cursor, key, val);
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
SetTable::sismember(str key, str val) {

	bool ret = false;

	ib_trx_t trx = 0;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	// get table cursor
	if(get_cursor(key, val, trx, cursor, row) == false) {
		return false;
	}

	// check if the row was found
	ret = (row != 0);

	// no write ops, rollback.
	rollback(trx, cursor, row);

	return ret;
}

bool
SetTable::get_cursor(str &key, str &val,
		ib_trx_t &trx, ib_crsr_t &cursor, ib_tpl_t &row) {

	ib_err_t err;
	ib_tpl_t search_row;

	// begin transaction
	trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	cursor = 0;
	if((err = ib_cursor_open_table_using_id(m_tid, trx, &cursor)) != DB_SUCCESS) {
		return false;
	}

	// create index cursor
	ib_crsr_t cursor_index;
	err = ib_cursor_open_index_using_name(cursor, "UNIQUE_KEY", &cursor_index);

	// create seach tuple, handling update case.
	search_row = ib_sec_search_tuple_create(cursor_index);

	// set key column
	err = ib_col_set_value(search_row, 0, key.c_str(), key.size());

	// set val column
	err = ib_col_set_value(search_row, 1, val.c_str(), val.size());

	// look for existing key
	int pos = -1;
	err = ib_cursor_moveto(cursor_index, search_row, IB_CUR_GE, &pos);

	// error handling
	if(err != DB_SUCCESS && err != DB_END_OF_INDEX) {
		err = ib_cursor_close(cursor_index);
		err = ib_cursor_close(cursor);
		rollback(trx, 0, search_row);
		cursor = 0;
		return false;
	}
	ib_tuple_delete(search_row); // no need for it anymore, we're positionned.

	if(pos == 0) { // return existing row.
		row = ib_clust_read_tuple_create(cursor_index);

		// read existing row
		if((err = ib_cursor_read_row(cursor_index, row)) != DB_SUCCESS) {
			// failure, close both cursor and bail.
			err = ib_cursor_close(cursor_index);
			err = ib_cursor_close(cursor);
			rollback(trx, 0, row);
			cursor = 0;
			row = 0;
			return false;
		}
	} else {
		// no row found, but cursor in place.
		row = 0;
	}

	// close original cursor
	err = ib_cursor_close(cursor);

	// return index cursor, ready for insertion.
	cursor = cursor_index;

	return true;
}

bool
SetTable::smembers(str key, std::vector<str> &out) {

	ib_err_t err;
	ib_tpl_t search_row = 0, row = 0;

	// begin transaction
	ib_trx_t trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	ib_crsr_t cursor = 0;
	if((err = ib_cursor_open_table_using_id(m_tid, trx, &cursor)) != DB_SUCCESS) {
		cerr << ib_strerror(err) << endl;
	}

	// create seach tuple, handling update case.
	search_row = ib_clust_search_tuple_create(cursor);

	// set key column
	err = ib_col_set_value(search_row, 0, key.c_str(), key.size());

	// look for existing key
	int pos = -1;
	err = ib_cursor_moveto(cursor, search_row, IB_CUR_GE, &pos);

	// error case
	if(err != DB_SUCCESS && err != DB_END_OF_INDEX) {
		ib_tuple_delete(search_row);
		rollback(trx, cursor, search_row);
		return false;
	}

	bool must_stop = false;
	while(err != DB_END_OF_INDEX && !must_stop) {

		// create read tuple
		row = ib_clust_read_tuple_create(cursor);

		// read existing row
		if((err = ib_cursor_read_row(cursor, row)) != DB_SUCCESS) {
			rollback(trx, cursor, row);
			return false;
		}
		str col0((const char*)ib_col_get_value(row, 0), ib_col_get_len(row, 0));
		if(col0 != key) {	// went too far.
			must_stop = true;
		} else {
			out.push_back(str((const char*)ib_col_get_value(row, 1), ib_col_get_len(row, 1), 1));
		}

		row = ib_tuple_clear(row);
		err = ib_cursor_next(cursor);
	}

	if(row) ib_tuple_delete(row);

	rollback(trx, cursor, search_row);
	return true;
}

bool
SetTable::srem(str key, str val) {

	ib_trx_t trx = 0;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	// get table cursor
	if(get_cursor(key, val, trx, cursor, row) == false) {
		return false;
	}

	// check if the row was found
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
