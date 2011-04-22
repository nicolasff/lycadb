#include "store.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>

using namespace std;

Store::Store(string db, string table) :
	m_db(db), m_table(db + "/" + table) {
}

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
	err = ib_cfg_set_bool_on("adaptive_hash_index");
	err = ib_cfg_set_bool_on("adaptive_flushing");

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
	return (ib_database_create(m_db.c_str()) == IB_TRUE);
}

bool
Store::createTable() {

	ib_err_t err;
	ib_tbl_sch_t schema = NULL;

	// create schema
	if((err = ib_table_schema_create(m_table.c_str(), &schema, IB_TBL_COMPACT, 0)) != DB_SUCCESS) {
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
Store::incr(string key, int by) {

	bool ret = false;
	ib_trx_t trx = 0;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(get_cursor(key, trx, cursor, row) == false) {
		return false;
	}

	if(row) {	// increment
		stringstream ss;
		int i = 0;

		// read value
		string s((const char*)ib_col_get_value(row, 1), ib_col_get_len(row, 1));
		if(s.empty()) {
			i = 0;
		} else {
			ss << s;

			// convert to number and increment
			ss >> i;
			ss.clear();
			ss.str("");
		}

		// convert back to string and update row.
		ss << (i + by);
		ret = update_row(cursor, row, ss.str());

	} else { // insert with value = "1".
		ret = insert_row(cursor, key, "1");
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
Store::decr(string key, int by) {
	return incr(key, -by);
}

bool
Store::update_row(ib_crsr_t cursor, ib_tpl_t row, string val) {
	
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
Store::insert_row(ib_crsr_t cursor, string key, string val) {
	
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

void
Store::commit(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row) {
	
	ib_err_t err;

	if(cursor) err = ib_cursor_close(cursor);
	if(row) ib_tuple_delete(row);
	if(trx) err = ib_trx_commit(trx);
}

void
Store::rollback(ib_trx_t trx, ib_crsr_t cursor, ib_tpl_t row) {
	ib_err_t err;

	if(cursor) err = ib_cursor_close(cursor);
	if(row) ib_tuple_delete(row);
	if(trx) err = ib_trx_commit(trx);
}

bool
Store::get_cursor(string &key, ib_trx_t &trx, ib_crsr_t &cursor, ib_tpl_t &row) {

	ib_err_t err;
	ib_tpl_t search_row;
	
	// begin transaction
	trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	cursor = 0;
	if((err = ib_cursor_open_table(m_table.c_str(), trx, &cursor)) != DB_SUCCESS) {
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
Store::set(string k, string v) {

	ib_err_t err;
	
	// begin transaction
	ib_trx_t trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	ib_crsr_t cursor = NULL;
	if((err = ib_cursor_open_table(m_table.c_str(), trx, &cursor)) != DB_SUCCESS) {
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

		// compare to new value
		if(ib_col_get_len(old_row, 1) == v.size() &&
			::memcmp(ib_col_get_value(old_row, 1), v.c_str(), v.size()) == 0) {
			// same value!

			err = ib_cursor_close(cursor);
			ib_tuple_delete(old_row);
			err = ib_trx_rollback(trx);
			return true;
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
Store::get(string k, string &v) {

	ib_err_t err;
	
	// begin transaction
	ib_trx_t trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open cursor
	ib_crsr_t cursor = NULL;
	if((err = ib_cursor_open_table(m_table.c_str(), trx, &cursor)) != DB_SUCCESS) {
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

