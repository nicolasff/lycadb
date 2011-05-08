#include "listtable.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>
#include <cstdlib>	// srand, rand.

using namespace std;

ListTable::ListTable(string name) : Table(name) {
}

bool
ListTable::create() {

	ib_err_t err;
	ib_tbl_sch_t schema = NULL;

	// create schema
	if((err = ib_table_schema_create(m_name.c_str(), &schema, IB_TBL_COMPACT, 0)) != DB_SUCCESS) {
		return false;
	}

	// add `id` column as INT64 (8 bytes)
	if((err = ib_table_schema_add_col(schema, "id", IB_INT, IB_COL_UNSIGNED, 0, 8)) != DB_SUCCESS) {
		return false;
	}

	// add `val` column as BLOB
	if((err = ib_table_schema_add_col(schema, "val", IB_BLOB, IB_COL_NOT_NULL, 0, 0)) != DB_SUCCESS) {
		return false;
	}

	// add `prev` column as INT64 (8 bytes)
	if((err = ib_table_schema_add_col(schema, "prev", IB_INT, IB_COL_UNSIGNED, 0, 8)) != DB_SUCCESS) {
		return false;
	}

	// add `next` column as INT64 (8 bytes)
	if((err = ib_table_schema_add_col(schema, "next", IB_INT, IB_COL_UNSIGNED, 0, 8)) != DB_SUCCESS) {
		return false;
	}

	if(!create_unique_index(schema)) {
		return false;
	}

	// begin transaction
	return install_schema(schema);
}

bool
ListTable::create_unique_index(ib_tbl_sch_t &schema) {

	ib_err_t err;

	// create primary key index.
	ib_idx_sch_t unique_index = NULL;
	if((err = ib_table_schema_add_index(schema, "UNIQUE_ID", &unique_index)) != DB_SUCCESS) {
		return false;
	}

	// add `key` column to index.
	if((err = ib_index_schema_add_col(unique_index, "id", 0)) != DB_SUCCESS) {
		return false;
	}

	// set unique index.
	if((err = ib_index_schema_set_clustered(unique_index)) != DB_SUCCESS) {
		return false;
	}

	return true;
}

uint64_t
ListTable::insert_row(ib_trx_t trx, str val, uint64_t prev, uint64_t next) {

	ib_err_t err;

	// open cursor
	ib_crsr_t cursor = 0;
	if((err = ib_cursor_open_table_using_id(m_tid, trx, &cursor)) != DB_SUCCESS) {
		cerr << ib_strerror(err) << endl;
	}

	// create row
	ib_tpl_t row = ib_clust_read_tuple_create(cursor);

	// generate row id
	uint64_t id = ((uint64_t)rand() << 32) + (uint64_t)rand();

	// set id, val, prev, next columns
	err = ib_tuple_write_u64(row, 0, id);
	err = ib_col_set_value(row, 1, val.c_str(), val.size());
	err = ib_tuple_write_u64(row, 2, prev);
	err = ib_tuple_write_u64(row, 2, next);

	// insert row
	bool ret = (ib_cursor_insert_row(cursor, row) == DB_SUCCESS);

	// delete row object
	ib_tuple_delete(row);

	// close cursor
	err = ib_cursor_close(cursor);

	if(ret) return id;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////


ListHeadTable::ListHeadTable(string name) : Table(name), m_lists(name + "-data") {
	srand(time(0));
}

bool
ListHeadTable::create() {

	ib_err_t err;
	ib_tbl_sch_t schema = NULL;

	// create schema
	if((err = ib_table_schema_create(m_name.c_str(), &schema, IB_TBL_COMPACT, 0)) != DB_SUCCESS) {
		return false;
	}

	// add `key` column as VARCHAR(64).
	if((err = ib_table_schema_add_col(schema, "key", IB_VARCHAR, IB_COL_NOT_NULL, 0, 64)) != DB_SUCCESS) {
		return false;
	}

	// add `head` column as INT64 (8 bytes).
	if((err = ib_table_schema_add_col(schema, "head", IB_INT, IB_COL_UNSIGNED, 0, 8)) != DB_SUCCESS) {
		return false;
	}

	// add `tail` column as INT64 (8 bytes).
	if((err = ib_table_schema_add_col(schema, "tail", IB_INT, IB_COL_UNSIGNED, 0, 8)) != DB_SUCCESS) {
		return false;
	}

	// add `val` column as INT64 (8 bytes).
	if((err = ib_table_schema_add_col(schema, "count", IB_INT, IB_COL_UNSIGNED, 0, 8)) != DB_SUCCESS) {
		return false;
	}

	if(!create_unique_index(schema)) {
		return false;
	}

	// begin transaction
	return install_schema(schema) && m_lists.create();
}

bool
ListHeadTable::create_unique_index(ib_tbl_sch_t &schema) {

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

	// set unique index.
	if((err = ib_index_schema_set_clustered(unique_index)) != DB_SUCCESS) {
		return false;
	}

	return true;
}

bool
ListHeadTable::lpush(str key, str val, int &out) {

	ib_err_t err;

	// begin transaction
	ib_trx_t trx = ib_trx_begin(IB_TRX_REPEATABLE_READ);

	// open head table
	ib_crsr_t cursor = 0;
	if((err = ib_cursor_open_table_using_id(m_tid, trx, &cursor)) != DB_SUCCESS) {
		err = ib_trx_rollback(trx);
		return false;
	}

	// create search tuple.
	ib_tpl_t search_row = ib_clust_search_tuple_create(cursor);

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
	
	uint64_t id = 0;
	uint64_t cur_head = 0, cur_tail = 0, cur_count = 0;
	ib_tpl_t row = 0;

	if(pos == 0) {	// list exists, read head.

		// read row
		row = ib_clust_read_tuple_create(cursor);
		if((err = ib_cursor_read_row(cursor, row)) != DB_SUCCESS) {
			return false;
		}

		// extract values
		err = ib_tuple_read_u64(row, 1, &cur_head);
		err = ib_tuple_read_u64(row, 2, &cur_tail);
		err = ib_tuple_read_u64(row, 3, &cur_count);

		cout << "cur_head=" << cur_head << 
			", cur_tail=" << cur_tail << 
			", cur_count=" << cur_count << endl;
	}

	// insert row before the head that was found, or with next=0 otherwise.
	do {
		id = m_lists.insert_row(trx, val, 0, cur_head);
	} while(!id);
	cout << "id=" << id << endl;


	if(err == DB_END_OF_INDEX || pos != 0) { // create new list
		if(insert_row(cursor, key, id, id, 1)) {
			cout << "NEW LIST CREATED" << endl;
			out = 1;
		} else {
			cout << "NEW LIST FAIL" << endl;
		}
	} else {	// update list
		cout << "UPDATING..." << endl;

		// update head, change its prev pointer to the new id. (TODO).

		// update key in order to point to the new head.
		if(update_row(cursor, row, id, cur_tail, cur_count + 1)) {
			cout << "LIST UPDATED" << endl;
			out = cur_count + 1;
		} else {
			cerr << ib_strerror(err) << endl;
			cout << "LIST UPDATE FAIL" << endl;
		}
	}

	if(row) ib_tuple_delete(row);			// delete row
	if(cursor) err = ib_cursor_close(cursor);	// close cursor
	if(trx) err = ib_trx_commit(trx);		// commit trx

	return false;
}

bool
ListHeadTable::insert_row(ib_crsr_t cursor, str key, uint64_t head, uint64_t tail, uint64_t count) {

	ib_err_t err;

	// create row
	ib_tpl_t row = ib_clust_read_tuple_create(cursor);

	// set key, head, tail, count columns
	err = ib_col_set_value(row, 0, key.c_str(), key.size());
	err = ib_tuple_write_u64(row, 1, head);
	err = ib_tuple_write_u64(row, 2, tail);
	err = ib_tuple_write_u64(row, 3, count);

	// insert row
	bool ret = (ib_cursor_insert_row(cursor, row) == DB_SUCCESS);

	// delete row object
	ib_tuple_delete(row);

	return ret;
}

bool
ListHeadTable::update_row(ib_crsr_t cursor, ib_tpl_t row, uint64_t head, uint64_t tail, uint64_t count) {

	ib_err_t err;

	// compare to new value
	uint64_t cur_head, cur_tail, cur_count;
	err = ib_tuple_read_u64(row, 1, &cur_head);
	err = ib_tuple_read_u64(row, 2, &cur_tail);
	err = ib_tuple_read_u64(row, 3, &cur_count);

	if(cur_head == head && cur_tail == tail && cur_count == count) {
		// same value!
		return true;
	}

	// update values
	cout << "create new row" << endl;
	ib_tpl_t new_row = ib_clust_read_tuple_create(cursor);
	err = ib_tuple_copy(new_row, row);
	err = ib_tuple_write_u64(new_row, 1, head);
	err = ib_tuple_write_u64(new_row, 2, tail);
	err = ib_tuple_write_u64(new_row, 3, count);

	// insert new value in place.
	if((err = ib_cursor_update_row(cursor, row, new_row)) != DB_SUCCESS) {
		ib_tuple_delete(new_row);
		return false;
	}

	// delete row object
	ib_tuple_delete(new_row);
	return true;
}

