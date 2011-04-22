#include "store.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>

using namespace std;

Store::Store(string db) :
	m_db(db), m_table(db + "/main") {
}

bool
Store::startup() {

	ib_err_t err;

	// initialize HailDB.
	err = ib_init();

	// configure a few options
	err = ib_cfg_set("data_home_dir", "/tmp/");
	err = ib_cfg_set("log_group_home_dir", "/tmp/");
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

	return this->createDb() && m_table.create();
}

bool
Store::createDb() {
	return (ib_database_create(m_db.c_str()) == IB_TRUE);
}

