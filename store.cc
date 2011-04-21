#include "store.h"

#include <string>
#include <iostream>

#include <haildb.h>

using namespace std;

bool
Store::startup() {

	ib_err_t err;

	// initialize HailDB.
	err = ib_init();

	// configure a few options
	err = ib_cfg_set("data_home_dir", "./datafiles/");
	err = ib_cfg_set("log_group_home_dir", "./datafiles/");
	err = ib_cfg_set("buffer_pool_size", 512);
	err = ib_cfg_set("flush_log_at_trx_commit", 2);

	// actually start HailDB
	err = ib_startup("barracuda");

	return (err == DB_SUCCESS);
}

bool
Store::shutdown() {

	ib_err_t err;
	if((err = ib_shutdown(IB_SHUTDOWN_NORMAL)) != DB_SUCCESS) {
		return false;
	}

	return true;
}

bool
Store::install() {

	return this->createDb() && this->createTable();
}

bool
Store::createDb() {
	return (ib_database_create("db0") == IB_TRUE);
}

bool
Store::createTable() {

	ib_err_t err;
	ib_tbl_sch_t schema = NULL;

	// create schema
	if((err = ib_table_schema_create("db0/main", &schema, IB_TBL_COMPACT, 0)) != DB_SUCCESS) {
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

	// create primary key index.
	ib_idx_sch_t index = NULL;
	if((err = ib_table_schema_add_index(schema, "PRIMARY_KEY", &index)) != DB_SUCCESS) {
		return false;
	}

	// add `key` column to primary index.
	if((err = ib_index_schema_add_col(index, "key", 0)) != DB_SUCCESS) {
		return false;
	}
	// set clustered index.
	if((err = ib_index_schema_set_clustered(index)) != DB_SUCCESS) {
		return false;
	}


	ib_id_t tid = 0; // table id

	// begin transaction
	bool ret = true;
	ib_trx_t trx = ib_trx_begin(IB_TRX_SERIALIZABLE);
	if((err = ib_schema_lock_exclusive(trx)) != DB_SUCCESS) {
		ret = false;
	}
	if((err = ib_table_create(trx, schema, &tid)) != DB_SUCCESS) {
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

bool
Store::set(string &k, string &v) {

	ib_err_t err;
	
	// begin transaction
	ib_trx_t trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	ib_crsr_t cursor = NULL;
	if((err = ib_cursor_open_table("db0/main", trx, &cursor)) != DB_SUCCESS) {
		cerr << ib_strerror(err) << endl;
	}

	// create seach tuple, handling update case.
	ib_tpl_t search_tpl = ib_clust_search_tuple_create(cursor);

	// set key column
	err = ib_col_set_value(search_tpl, 0, k.c_str(), k.size());

	// look for existing key
	int pos = -1;
	err = ib_cursor_moveto(cursor, search_tpl, IB_CUR_GE, &pos);
	ib_tuple_delete(search_tpl);

	// error handling
	if(err != DB_SUCCESS && err != DB_END_OF_INDEX) {
		err = ib_cursor_close(cursor);
		err = ib_trx_rollback(trx);
		return false;
	}

	if(pos == 0) { // update existing row.
		ib_tpl_t old_row = ib_clust_read_tuple_create(cursor);

		// read existing row
		if((err = ib_cursor_read_row(cursor, old_row)) != DB_SUCCESS) {
			err = ib_cursor_close(cursor);
			ib_tuple_delete(old_row);
			err = ib_trx_rollback(trx);
			return false;
		}

		// update value
		ib_tpl_t new_row = ib_clust_read_tuple_create(cursor);
		err = ib_tuple_copy(new_row, old_row);
		err = ib_col_set_value(new_row, 1, v.c_str(), v.size());

		// insert new value in place.
		if((err = ib_cursor_update_row(cursor, old_row, new_row)) != DB_SUCCESS) {
			err = ib_cursor_close(cursor);
			ib_tuple_delete(old_row);
			ib_tuple_delete(new_row);
			err = ib_trx_rollback(trx);
			return false;
		}

		// delete row object
		ib_tuple_delete(new_row);
		ib_tuple_delete(old_row);

	} else { // no existing row, insert new.

		ib_tpl_t new_row = ib_clust_read_tuple_create(cursor);
		
		// set key and val columns
		err = ib_col_set_value(new_row, 0, k.c_str(), k.size());
		err = ib_col_set_value(new_row, 1, v.c_str(), v.size());

		// insert row
		if((err = ib_cursor_insert_row(cursor, new_row)) != DB_SUCCESS) {
			err = ib_cursor_close(cursor);
			ib_tuple_delete(new_row);
			err = ib_trx_rollback(trx);
			return false;
		}

		// delete row object
		ib_tuple_delete(new_row);
	}
	
	// close cursor
	if((err = ib_cursor_close(cursor)) != DB_SUCCESS) {
		err = ib_trx_rollback(trx);
		return false;
	}

	// commit transaction
	if((err = ib_trx_commit(trx)) != DB_SUCCESS) {
		return false;
	}
	return true;
}

bool
Store::get(string &k, string &v) {

	ib_err_t err;
	
	// begin transaction
	ib_trx_t trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	ib_crsr_t cursor = NULL;
	if((err = ib_cursor_open_table("db0/main", trx, &cursor)) != DB_SUCCESS) {
		cerr << ib_strerror(err) << endl;
	}

	// create tuple
	ib_tpl_t search_tpl = ib_clust_search_tuple_create(cursor);

	// set key column
	if((err = ib_col_set_value(search_tpl, 0, k.c_str(), k.size())) != DB_SUCCESS) {
		cerr << "0:" << ib_strerror(err) << endl;
	}

	// move cursor
	int res;
	if((err = ib_cursor_moveto(cursor, search_tpl, IB_CUR_GE, &res)) != DB_SUCCESS) {
		cerr << "m:" << ib_strerror(err) << endl;
	}
	ib_tuple_delete(search_tpl);
	if(res != 0) {	// key not found
		err = ib_cursor_close(cursor);
		err = ib_trx_commit(trx);
		return false;
	}

	ib_tpl_t row_tpl = ib_clust_read_tuple_create(cursor);
	// read row
	if((err = ib_cursor_read_row(cursor, row_tpl)) != DB_SUCCESS) {
		cerr << ib_strerror(err) << endl;
	}

	// extract string
	int sz;
	const void *ptr;
	sz = ib_col_get_len(row_tpl, 1);
	ptr = ib_col_get_value(row_tpl, 1);
	v = string((const char *)ptr, sz);

	// close cursor
	if((err = ib_cursor_close(cursor)) != DB_SUCCESS) {
		cerr << "c:" << ib_strerror(err) << endl;
	}

	// commit transaction
	if((err = ib_trx_commit(trx)) != DB_SUCCESS) {
		cerr << ib_strerror(err) << endl;
	}
	ib_tuple_delete(row_tpl);
	return true;
}

