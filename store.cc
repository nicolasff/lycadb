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
	err = ib_cfg_set("log_file_size", 128);
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
Store::get(string key, string &val) {
	return m_main.get(key, val);
}

bool
Store::set(string key, string val) {
	return m_main.set(key, val);
}

// increment
bool
Store::incr(string key, int by){
	return m_main.incr(key, by);
}

bool
Store::decr(string key, int by) {
	return m_main.decr(key, by);
}

bool
Store::del(string key) {
	return m_main.del(key);
}

bool
Store::flushall() {
	m_main.flushall();
	m_sets.flushall();
	return true;
}

bool
Store::sadd(string key, string val) {
	return m_sets.sadd(key, val);
}

bool
Store::sismember(string key, string val) {
	return m_sets.sismember(key, val);
}

bool
Store::smembers(string key, vector<string> &out) {
	return m_sets.smembers(key, out);
}

bool
Store::srem(string key, string val) {
	return m_sets.srem(key, val);
}
