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

	if(!create_primary_index(schema)) {
		return false;
	}
	
	if(!create_unique_index(schema)) {
		return false;
	}

	// begin transaction
	return install_schema(schema);
}

bool
SetTable::create_primary_index(ib_tbl_sch_t &schema) {
	
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

bool
SetTable::create_unique_index(ib_tbl_sch_t &schema) {

	ib_err_t err;
	
	// create primary key index.
	ib_idx_sch_t unique_index = NULL;
	if((err = ib_table_schema_add_index(schema, "UNIQUE_KEY", &unique_index)) != DB_SUCCESS) {
		cout << "sets:0:" << ib_strerror(err) << endl;
		return false;
	}

	// add `key` column to index.
	if((err = ib_index_schema_add_col(unique_index, "key", 0)) != DB_SUCCESS) {
		cout << "sets:1:" << ib_strerror(err) << endl;
		return false;
	}

	// add `val` column to index.
	if((err = ib_index_schema_add_col(unique_index, "val", 0)) != DB_SUCCESS) {
		cout << "sets:2:" << ib_strerror(err) << endl;
		return false;
	}

	// set unique index.
	if((err = ib_index_schema_set_unique(unique_index)) != DB_SUCCESS) {
		cout << "sets:3:" << ib_strerror(err) << endl;
		return false;
	}

	return true;
}

bool
SetTable::sadd(string key, string val) {

	return false;
}
