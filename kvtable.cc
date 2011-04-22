#include "kvtable.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>

using namespace std;

KVTable::KVTable(string name) : Table(name) {

}

bool
KVTable::create() {

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
KVTable::set(string key, string val) {

	bool ret = false;

	ib_trx_t trx = 0;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	// get table cursor
	if(get_cursor(key, trx, cursor, row) == false) {
		return false;
	}

	if(row) { // existing value

		string old((const char*)ib_col_get_value(row, 1), ib_col_get_len(row, 1));

		if(old == val) {	// no need to update
			rollback(trx, cursor, row);
			return true;
		} else {
			// convert back to string and update row.
			ret = update_row(cursor, row, val);
		}


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
KVTable::get(string key, string &val) {

	ib_trx_t trx = 0;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	// get table cursor
	if(get_cursor(key, trx, cursor, row) == false) {
		return false;
	}

	if(row) { // found value

		// copy to output parameter
		val = string((const char*)ib_col_get_value(row, 1), ib_col_get_len(row, 1));

		commit(trx, cursor, row);
		return true;
	} else {
		rollback(trx, cursor, row);
		return false;
	}
}

bool
KVTable::incr(string key, int by) {

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
KVTable::decr(string key, int by) {
	return incr(key, -by);
}
