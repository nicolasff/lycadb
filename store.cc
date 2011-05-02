#include "store.h"

#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include <haildb.h>

using namespace std;

Store::Store(string db) :
	m_db(db), m_main(db + "/main"), m_sets(db + "/sets") {
}

bool
Store::startup() {

	ib_err_t err;

	// initialize HailDB.
	err = ib_init();

	// configure a few options
	err = ib_cfg_set("data_home_dir", "/tmp/");
	err = ib_cfg_set("log_group_home_dir", "/tmp/");
	err = ib_cfg_set("log_file_size", 128*1024*1024);
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

	return this->createDb()
		&& m_main.create()
		&& m_sets.create();
}

bool
Store::createDb() {
	return (ib_database_create(m_db.c_str()) == IB_TRUE);
}

// basic key management
bool
Store::get(str key, str *val) {
	return m_main.get(key, val);
}

bool
Store::set(str key, str val) {
	return m_main.set(key, val);
}

// increment
bool
Store::incr(str key, int by, int &out){
	return m_main.incr(key, by, out);
}

bool
Store::decr(str key, int by, int &out) {
	return m_main.decr(key, by, out);
}

bool
Store::del(str key) {
	return m_main.del(key);
}


bool
Store::sadd(str key, str val) {
	return m_sets.sadd(key, val);
}

bool
Store::sismember(str key, str val) {
	return m_sets.sismember(key, val);
}

bool
Store::smembers(str key, vector<str> &out) {
	return m_sets.smembers(key, out);
}

bool
Store::srem(str key, str val) {
	return m_sets.srem(key, val);
}

bool
Store::flushall() {
	m_main.flushall();
	m_sets.flushall();
	return true;
}
