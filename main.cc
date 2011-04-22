#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <haildb.h>
#include <pthread.h>

#include "store.h"

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

	Store s("db0");
	s.startup();
	s.install();
	cout << "Benchmarking, please wait." << endl;

	// start with an empty table.
	s.flushall();

	// typedef for a key-value pair.
	typedef pair<string,string> key_value_t;
	vector<key_value_t> kv_pairs;

	// generate key-value pairs.
	int n = 100000;
	for(int i = 0; i < n; ++i) {
		stringstream ssk, ssv;
		ssk << "key-" << i;
		ssv << "val-" << i;

		kv_pairs.push_back(make_pair(ssk.str(), ssv.str()));
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
			string val;
			s.get(kvi->first, val);
		}
	});

	BENCH("UPDATE", n, {
		// update all
		for(kvi = kv_pairs.begin(); kvi != kv_pairs.end(); kvi++) {
			s.set(kvi->first, kvi->second + "-new");
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
			s.incr(kvi->first);
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

	s.shutdown();

	return 0;
}

