#include "store.h"
#include "config.h"

#include <string>
#include <sstream>

#include <haildb.h>

using namespace std;

Store::Store(string db, Config &config) :
	m_db(db),
	m_main(db + "/main"),
	m_sets(db + "/sets"),
	m_lists(db + "/lists"),
	m_zsets(db + "/zsets"),
	m_config(config) {
}

bool
Store::startup() {

	ib_err_t err;

	// initialize HailDB.
	err = ib_init();

	// configure options.
	Config::const_iterator ci;
	for(ci = m_config.begin(); ci != m_config.end(); ci++) {
		const char * key = ci->first.c_str();
		string val = ci->second;

		if(val == "On") {	// bool, on
			err = ib_cfg_set_bool_on(key);
		} else if(val == "Off") { // bool, off
			err = ib_cfg_set_bool_off(key);
		} else if(val.find_first_not_of("0123456789") == string::npos) { // int
			err = ib_cfg_set(key, m_config.get<int>(key));
		} else {	// string
			err = ib_cfg_set(key, val.c_str());
		}
	}

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
Store::load() {

	return  m_main.load()
		&& m_sets.load()
		&& m_lists.load()
		&& m_zsets.load();
}

bool
Store::install() {

	return this->createDb()
		&& m_main.create()
		&& m_sets.create()
		&& m_lists.create()
		&& m_zsets.create();
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
Store::lpush(str key, str val, int &out) {
	return m_lists.lpush(key, val, out);
}

bool
Store::rpush(str key, str val, int &out) {
	return m_lists.rpush(key, val, out);
}

bool
Store::lrange(str key, int start, int stop, std::vector<str> &out) {
	return m_lists.lrange(key, start, stop, out);
}

bool
Store::llen(str key, int &out) {
	return m_lists.llen(key, out);
}

bool
Store::lpop(str key, str &val) {
	return m_lists.lpop(key, val);
}

bool
Store::rpop(str key, str &val) {
	return m_lists.rpop(key, val);
}

void
Store::ldump(str key) {
	m_lists.debug_dump(key);
}

bool
Store::zadd(str key, str val, double score)
{
	return m_zsets.zadd(key, val, score);
}

bool
Store::zcard(str key, int &out)
{
	return m_zsets.zcard(key, out);
}

bool
Store::flushall() {
	m_main.flushall();
	m_sets.flushall();
	return true;
}
