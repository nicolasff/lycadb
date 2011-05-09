#include "listtable.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>
#include <cstdlib>	// srand, rand.

using namespace std;
using tr1::get;

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

bool
ListTable::find_row(ib_trx_t trx, uint64_t id, ib_crsr_t &cursor) {

	ib_err_t err;

	// open cursor
	cursor = 0;
	if((err = ib_cursor_open_table_using_id(m_tid, trx, &cursor)) != DB_SUCCESS) {
		return false;
	}

	// create search tuple, handling update case.
	ib_tpl_t search_row = ib_clust_search_tuple_create(cursor);

	// set id column
	err = ib_tuple_write_u64(search_row, 0, id);

	// look for existing key
	int pos = -1;
	err = ib_cursor_moveto(cursor, search_row, IB_CUR_GE, &pos);

	ib_tuple_delete(search_row);	// positioned, no need to keep the search tuple.

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
	err = ib_tuple_write_u64(row, 3, next);

	// insert row
	bool ret = (ib_cursor_insert_row(cursor, row) == DB_SUCCESS);

	// delete row object
	ib_tuple_delete(row);

	// close cursor
	err = ib_cursor_close(cursor);

	// failure
	if(!ret) return 0;

	// look for prev and next, update them to point to the new row.
	if(prev) { // update prev→next
		update_row(trx, prev, ListTable::NEXT, id);
	}

	if(next) { // update next→prev
		update_row(trx, next, ListTable::PREV, id);
	}

	return id;
}

bool
ListTable::update_row(ib_trx_t trx, uint64_t id, int col_id, uint64_t val) {

	ib_err_t err;
	ib_crsr_t cursor = 0;

	if(!find_row(trx, id, cursor)) {
		return false;
	}

	// read existing row
	ib_tpl_t cur_row = ib_clust_read_tuple_create(cursor);
	err = ib_cursor_read_row(cursor, cur_row);	// TODO: check return value

	// new row
	ib_tpl_t new_row = ib_clust_read_tuple_create(cursor);
	err = ib_tuple_copy(new_row, cur_row);

	err = ib_tuple_write_u64(new_row, col_id, val); // change column value
	err = ib_cursor_update_row(cursor, cur_row, new_row);	// TODO: check return value.

	// close cursor
	err = ib_cursor_close(cursor);

	// cleanup
	ib_tuple_delete(cur_row);
	ib_tuple_delete(new_row);

	return true;
}

ListTable::RowData
ListTable::read(ib_crsr_t cursor) {
	ib_err_t err;

	ListTable::RowData rd;

	// read row
	ib_tpl_t row = ib_clust_read_tuple_create(cursor);
	if((err = ib_cursor_read_row(cursor, row)) != DB_SUCCESS) {
		ib_tuple_delete(row);
		return rd;
	}

	// extract fields
	uint64_t id = 0, prev = 0, next = 0;
	err = ib_tuple_read_u64(row, 0, &id);
	str s((const char*)ib_col_get_value(row, 1), ib_col_get_len(row, 1), 1);
	err = ib_tuple_read_u64(row, 2, &prev);
	err = ib_tuple_read_u64(row, 3, &next);

	rd = tr1::make_tuple(id, s, prev, next);

	ib_tuple_delete(row);

	return rd;
}

bool
ListTable::delete_row(ib_trx_t trx, uint64_t id) {

	ib_err_t err;
	ib_crsr_t cursor = 0;

	// get table cursor
	if(find_row(trx, id, cursor) == false) {
		return false;
	}

	bool ret = ((err = ib_cursor_delete_row(cursor)) == DB_SUCCESS);
	err = ib_cursor_close(cursor);

	return ret;
}


////////////////////////////////////////////////////////////////////////////////


ListHeadTable::ListHeadTable(string name) : Table(name), m_lists(name + "-data") {
	srand(time(0));
}

bool
ListHeadTable::load() {
	return Table::load() && m_lists.load();
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

	ib_trx_t trx;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(!get_cursor(key, trx, cursor, row)) {
		return false;
	}

	uint64_t id = 0, cur_head = 0, cur_tail = 0, cur_count = 0;
	ListHeadTable::RowData hrd;
	if(row != 0) {	// list exists, read head.
		hrd = read(cursor);
		cur_head = get<ListHeadTable::HEAD>(hrd);
		cur_tail = get<ListHeadTable::TAIL>(hrd);
		cur_count = get<ListHeadTable::COUNT>(hrd);

		// cleanup
		get<ListHeadTable::KEY>(hrd).reset();
	}

	// insert row before the head that was found, or with prev=next=0 otherwise.
	do {
		id = m_lists.insert_row(trx, val, 0, cur_head);
	} while(!id);


	if(row == 0) { // create new list
		insert_row(cursor, key, id, id, 1);
		out = 1;
	} else {	// update list

		// update head, change its prev pointer to the new id.
		m_lists.update_row(trx, cur_head, ListTable::PREV, id);

		// update key in order to point to the new head.
		update_row(cursor, row, id, cur_tail, cur_count + 1);
		out = cur_count + 1;
	}

	commit(trx, cursor, row);
	return true;
}

bool
ListHeadTable::rpush(str key, str val, int &out) {

	ib_trx_t trx;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(!get_cursor(key, trx, cursor, row)) {
		return false;
	}

	uint64_t id = 0, cur_head = 0, cur_tail = 0, cur_count = 0;
	ListHeadTable::RowData hrd;
	if(row != 0) {	// list exists, read head.
		hrd = read(cursor);
		cur_head = get<ListHeadTable::HEAD>(hrd);
		cur_tail = get<ListHeadTable::TAIL>(hrd);
		cur_count = get<ListHeadTable::COUNT>(hrd);

		// cleanup
		get<ListHeadTable::KEY>(hrd).reset();
	}

	// insert row after the tail that was found, or with prev=next=0 otherwise.
	do {
		id = m_lists.insert_row(trx, val, cur_tail, 0);
	} while(!id);


	if(row == 0) { // create new list
		insert_row(cursor, key, id, id, 1);
		out = 1;
	} else {	// update list

		// update tail, change its next pointer to the new id.
		m_lists.update_row(trx, cur_tail, ListTable::NEXT, id);

		// update key in order to point to the new tail.
		update_row(cursor, row, cur_head, id, cur_count + 1);
		out = cur_count + 1;
	}

	commit(trx, cursor, row);
	return true;
}

bool
ListHeadTable::lpop(str key, str &val) {

	ib_trx_t trx;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(!get_cursor(key, trx, cursor, row)) {
		return false;
	}

	uint64_t cur_head = 0, cur_tail = 0, cur_count = 0;
	ListHeadTable::RowData hrd;
	if(row != 0) {	// list exists, read head.
		hrd = read(cursor);
		cur_head = get<ListHeadTable::HEAD>(hrd);
		cur_tail = get<ListHeadTable::TAIL>(hrd);
		cur_count = get<ListHeadTable::COUNT>(hrd);

		// cleanup
		get<ListHeadTable::KEY>(hrd).reset();
	}


	// read first row.
	ib_crsr_t item_cursor;
	if(!m_lists.find_row(trx, cur_head, item_cursor)) {
		rollback(trx, cursor, row);
		return false;
	}

	// read data
	ListTable::RowData rd = m_lists.read(item_cursor);
	ib_err_t err = ib_cursor_close(item_cursor);
	(void)err;
	val = get<ListTable::VAL>(rd);
	uint64_t new_head = get<ListTable::NEXT>(rd), new_tail = cur_tail, new_count = cur_count - 1;

	// update new head, prev → 0
	m_lists.update_row(trx, new_head, ListTable::PREV, 0);

	// delete row.
	m_lists.delete_row(trx, cur_head);

	// update key in order to point to the new head, update count.
	if(new_head == 0) new_tail = 0;
	if(cur_count == 0) new_count = 0;
	update_row(cursor, row, new_head, new_tail, new_count);

	commit(trx, cursor, row);
	return true;
}

bool
ListHeadTable::rpop(str key, str &val) {

	ib_trx_t trx;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(!get_cursor(key, trx, cursor, row)) {
		return false;
	}

	uint64_t cur_head = 0, cur_tail = 0, cur_count = 0;
	ListHeadTable::RowData hrd;
	if(row != 0) {	// list exists, read head.
		hrd = read(cursor);
		cur_head = get<ListHeadTable::HEAD>(hrd);
		cur_tail = get<ListHeadTable::TAIL>(hrd);
		cur_count = get<ListHeadTable::COUNT>(hrd);

		// cleanup
		get<ListHeadTable::KEY>(hrd).reset();
	}


	// read first row.
	ib_crsr_t item_cursor;
	if(!m_lists.find_row(trx, cur_tail, item_cursor)) {
		rollback(trx, cursor, row);
		return false;
	}

	// read data
	ListTable::RowData rd = m_lists.read(item_cursor);
	ib_err_t err = ib_cursor_close(item_cursor);
	(void)err;
	val = get<ListTable::VAL>(rd);
	uint64_t new_tail = get<ListTable::PREV>(rd), new_head = cur_head, new_count = cur_count - 1;

	// update new tail, next → 0
	m_lists.update_row(trx, new_tail, ListTable::NEXT, 0);

	// delete row.
	m_lists.delete_row(trx, cur_tail);

	// update key in order to point to the new tail, update count.
	if(new_tail == 0) new_head = 0;
	if(cur_count == 0) new_count = 0;
	update_row(cursor, row, new_head, new_tail, new_count);

	commit(trx, cursor, row);
	return true;
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

bool
ListHeadTable::get_cursor(str &key, ib_trx_t &trx, ib_crsr_t &cursor, ib_tpl_t &row) {

	ib_err_t err;

	// begin transaction
	if((trx = ib_trx_begin(IB_TRX_REPEATABLE_READ)) == 0) {
		return false;
	}

	// open head table
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

	ib_tuple_delete(search_row); // no need for it anymore, we're positionned.

	// error handling
	if((err != DB_SUCCESS && err != DB_END_OF_INDEX) || pos != 0) {
		row = 0;
		return true;	// row doesn't exist.
	}

	// read row
	row = ib_clust_read_tuple_create(cursor);
	if((err = ib_cursor_read_row(cursor, row)) != DB_SUCCESS) {
		rollback(trx, cursor, 0);
		return false;
	}

	return true;
}

void
ListHeadTable::debug_dump(str key) {

	ib_err_t err;
	ib_trx_t trx;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(get_cursor(key, trx, cursor, row) == false) {
		return;
	}

	// go through all items of the list.

	uint64_t id = 0;
	uint64_t cur_head = 0, cur_tail = 0, cur_count = 0;
	// extract values
	err = ib_tuple_read_u64(row, 1, &cur_head);
	err = ib_tuple_read_u64(row, 2, &cur_tail);
	err = ib_tuple_read_u64(row, 3, &cur_count);

	err = ib_cursor_close(cursor);
	cursor = 0;

	cout << "cur_head=" << cur_head << ", cur_tail=" << cur_tail << ", cur_count=" << cur_count << endl;

	id = cur_head;
	while(1) {

		if(!id || !m_lists.find_row(trx, id, cursor)) {
			break;
		}

		// read row.
		ListTable::RowData rd = m_lists.read(cursor);

		cout << "(id=" << get<ListTable::ID>(rd)
			<< ", val=[" << get<ListTable::VAL>(rd).c_str()
			<< "], prev=" << get<ListTable::PREV>(rd)
			<< ", next=" << get<ListTable::NEXT>(rd) << ")" << endl;

		id = get<ListTable::NEXT>(rd);
		err = ib_cursor_close(cursor);
	}

	err = ib_trx_rollback(trx);
}

ListHeadTable::RowData
ListHeadTable::read(ib_crsr_t cursor) {

	ib_err_t err;

	ListHeadTable::RowData rd;

	// read row
	ib_tpl_t row = ib_clust_read_tuple_create(cursor);
	if((err = ib_cursor_read_row(cursor, row)) != DB_SUCCESS) {
		return rd;
	}

	// extract fields
	uint64_t head = 0, tail = 0, count = 0;
	str s((const char*)ib_col_get_value(row, 0), ib_col_get_len(row, 0), 1);
	err = ib_tuple_read_u64(row, 1, &head);
	err = ib_tuple_read_u64(row, 2, &tail);
	err = ib_tuple_read_u64(row, 3, &count);

	rd = tr1::make_tuple(s, head, tail, count);

	ib_tuple_delete(row);

	return rd;
}

bool
ListHeadTable::lrange(str key, int start, int stop, vector<str> &out) {

	ib_err_t err;
	ib_trx_t trx;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(start < 0) {
		return false;
	}

	if(get_cursor(key, trx, cursor, row) == false) {
		return true;	// no list → empty list.
	}

	if(row == 0) {	// no items
		rollback(trx, cursor, row);
		return true;
	}

	// read list meta-data
	ListHeadTable::RowData hrd = read(cursor);

	// cleanup
	ib_tuple_delete(row);
	get<ListHeadTable::KEY>(hrd).reset();
	err = ib_cursor_close(cursor);
	cursor = 0;

	// go through all items of the list.
	uint64_t id = get<ListHeadTable::HEAD>(hrd);
	uint64_t count = get<ListHeadTable::COUNT>(hrd);

	if(stop < 0) stop += count;

	if(stop < start) {	// no items
		rollback(trx, 0, 0);
		return true;
	}

	int cur_pos = 0;
	while(1) {

		if(!id || !m_lists.find_row(trx, id, cursor)) {
			break;
		}

		// read row.
		ListTable::RowData rd = m_lists.read(cursor);
		err = ib_cursor_close(cursor);

		// advance id to the next row
		id = get<ListTable::NEXT>(rd);

		// when in interval, push data
		if(cur_pos >= start && (stop == -1 || cur_pos <= stop)) {
			out.push_back(get<ListTable::VAL>(rd));
		} else { // cleanup otherwise.
			get<ListTable::VAL>(rd).reset();
		}

		cur_pos++;

		if(cur_pos > stop) {	 // too far
			break;
		}
	}

	err = ib_trx_rollback(trx);

	return true;
}

bool
ListHeadTable::llen(str key, int &out) {

	ib_err_t err;
	ib_trx_t trx;
	ib_crsr_t cursor = 0;
	ib_tpl_t row = 0;

	if(get_cursor(key, trx, cursor, row) == false) {
		return false;
	}

	if(row == 0) {	// no items
		out = 0;
		return true;
	}

	// read list meta-data
	ListHeadTable::RowData hrd = read(cursor);

	// retrieve number of elements
	out = get<ListHeadTable::COUNT>(hrd);

	// cleanup
	err = ib_cursor_close(cursor);
	get<ListHeadTable::KEY>(hrd).reset();
	ib_tuple_delete(row);

	err = ib_trx_rollback(trx);

	return true;
}
