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

////////////////////////////////////////////////////////////////////////////////


ListHeadTable::ListHeadTable(string name) : Table(name) {
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

	// add `val` column as INT64 (8 bytes).
	if((err = ib_table_schema_add_col(schema, "count", IB_INT, IB_COL_UNSIGNED, 0, 8)) != DB_SUCCESS) {
		return false;
	}

	if(!create_unique_index(schema)) {
		return false;
	}

	// begin transaction
	return install_schema(schema);
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
