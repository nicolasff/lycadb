#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <haildb.h>
#include <pthread.h>

#include "store.h"
#include "net.h"
#include "config.h"

using namespace std;

#define BENCH(name, n, block) \
	{	struct timespec t0, t1;\
		clock_gettime(CLOCK_MONOTONIC, &t0);\
		block\
		clock_gettime(CLOCK_MONOTONIC, &t1);\
		float mili0 = t0.tv_sec * 1000 + t0.tv_nsec / 1000000;\
		float mili1 = t1.tv_sec * 1000 + t1.tv_nsec / 1000000;\
		cout << name << ": " << (mili1-mili0) / 1000 << " sec. (" << (1000 * n / (mili1-mili0)) << "/sec)" << endl;\
	}

int
main() {
	// quick and unscientific benchmarking.

	Config cfg;
	cfg.read("lycadb.conf");
	Store s("db0", cfg);
	s.startup();
	s.install();
#if 0
	Server srv("127.0.0.1", 1111, cfg.get<int>("threads"), s);
	srv.start();
#else
	cout << "Benchmarking, please wait." << endl;

	// start with an empty table.
	s.flushall();

	for(int i = 0; i < 10; ++i) {
		int tmp = 0;
		cout << s.lpush("x", "y", tmp) << endl;
		cout << "len=" << tmp << endl;
	}
	s.ldump("x");

	return 0;

	// typedef for a key-value pair.
	typedef pair<str,str> key_value_t;
	vector<key_value_t> kv_pairs;

	// generate key-value pairs.
	int n = 100000;
	for(int i = 0; i < n; ++i) {
		stringstream ssk, ssv;
		ssk << "key-" << i;
		ssv << "val-" << i;

		string key = ssk.str(), val = ssv.str();

		kv_pairs.push_back(make_pair(
					str(key.c_str(), key.size(), 1),
					str(val.c_str(), val.size(), 1)));
	}

	// iterator
	vector<key_value_t>::const_iterator kvi;

	BENCH("SET", n, {
		// insert all
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			s.set(kvi->first, kvi->second);
		}
	});

	// read back all the data.
	BENCH("GET", n, {
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			str val;
			s.get(kvi->first, &val);
			val.reset();
		}
	});

	BENCH("UPDATE", n, {
		// update all
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			string newval = kvi->second.c_str();
			newval += "-new";
			s.set(kvi->first, newval.c_str());
		}
	});

	BENCH("DEL", n, {
		// delete all.
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			s.del(kvi->first);
		}
	});

	BENCH("INCR", n, {
		// incr all.
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			int out;
			s.incr(kvi->first, 1, out);
		}
	});

	BENCH("SISMEMBER, returning false", n, {
		// incr all.
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			s.sismember(kvi->first, kvi->second);
		}
	});

	BENCH("SADD, new values", n, {
		// incr all.
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			s.sadd(kvi->first, kvi->second);
		}
	});

	BENCH("SADD, existing values", n, {
		// incr all.
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			s.sadd(kvi->first, kvi->second);
		}
	});

	BENCH("SISMEMBER, returning true", n, {
		// incr all.
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			s.sismember(kvi->first, kvi->second);
		}
	});

	s.shutdown();
#endif

	return 0;
}

