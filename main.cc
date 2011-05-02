#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <haildb.h>
#include <pthread.h>

#include "store.h"
#include "protocol.h"
#include "cmd.h"
#include "net.h"

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

void
on_cmd(Command *c) {
	delete c;
}
void
on_failure() {
	//cout << "ON FAILURE" << endl;
}

int
main() {
	// quick and unscientific benchmarking.

	/*
	Parser p;
	p.setSuccessCb(on_cmd);
	p.setFailureCb(on_failure);
	char str[] = "*2\r\n$3\r\nGET\r\n$5\r\nhello\r\n";
	for(int i = 0; i < 600*1000; ++i) {
		p.consume(str, sizeof(str)-1);
	}

	return 0;
	*/

	Store s("db0");
	s.startup();
	s.install();
	Server srv("127.0.0.1", 1111, s);
	srv.start();

	return 0;

	cout << "Benchmarking, please wait." << endl;

	// start with an empty table.
	s.flushall();

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

#if 1
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
#endif

	s.shutdown();

	return 0;
}

