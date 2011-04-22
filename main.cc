#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <haildb.h>
#include <pthread.h>

#include "store.h"

using namespace std;

void* thread_main(void *p);

class Thread {

public:
	void start() {
		pthread_create(&self, NULL, thread_main, this);
	}

	void wait() {
		pthread_join(self, NULL);
	}

	virtual void run() {}


private:
	pthread_t self;
};

class Writer : public Thread {

public:
	Writer(Store &s, int from, int to): m_store(s), m_from(from), m_to(to) {}

	void run() {

		for(int i = m_from; i < m_to; ++i) {
			stringstream ss;
			string k, v;

			ss << "key-" << i;
			k = ss.str();

			ss.str("");
			ss << "val-" << i;
			v = ss.str();

			if(i % 1000 == 0) {
				cout << "set(" << k << ", " << v << ")" << endl;
			}
			m_store.set(k, v);
		}
	}
private:

	Store &m_store;
	int m_from, m_to;
};

class Reader : public Thread {
public:
	Reader(Store &s, int from, int to): m_store(s), m_from(from), m_to(to) {}
	void run() {

		for(int i = m_from; i < m_to; ++i) {
			stringstream ss;
			string k;

			ss << "key-" << i;
			k = ss.str();

			string v = "?";
			m_store.get(k, v);
			if(i % 1000 == 0) {
				cout << k << ": [" << v << "]" << endl;
			}
		}
	}
private:
	Store &m_store;
	int m_from, m_to;
};


void*
thread_main(void *p) {

	Writer *w = static_cast<Writer*>(p);
	w->run();
	return NULL;
}


int
main() {
	Store s("db0", "main");
	s.startup();
	s.install();

#if 0
	struct timespec t0, t1, t2;
	clock_gettime(CLOCK_MONOTONIC, &t0);	// timing

	int n = 100000;
	int threads = 1;

	// n writes
	vector<Writer*> ws;
	for(int i = 0, cur = 0; i < threads; ++i, cur += n/threads) {
		Writer *w = new Writer(s, cur, cur + n/threads);
		w->start();
		ws.push_back(w);
	}
	for(vector<Writer*>::iterator wi = ws.begin(); wi != ws.end(); wi++) {
		(*wi)->wait();
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);

	// n reads
	vector<Reader*> rs;
	for(int i = 0, cur = 0; i < threads; ++i, cur += n/threads) {
		Reader *r = new Reader(s, cur, cur + n/threads);
		r->start();
		rs.push_back(r);
	}
	for(vector<Reader*>::iterator ri = rs.begin(); ri != rs.end(); ri++) {
		(*ri)->wait();
	}

	clock_gettime(CLOCK_MONOTONIC, &t2);

	float mili0 = t0.tv_sec * 1000 + t0.tv_nsec / 1000000;
	float mili1 = t1.tv_sec * 1000 + t1.tv_nsec / 1000000;
	float mili2 = t2.tv_sec * 1000 + t2.tv_nsec / 1000000;

	cout << "SET: " << (mili1-mili0) / 1000 << " sec. (" << (1000 * n / (mili1-mili0)) << "/sec)" << endl;
	cout << "GET: " << (mili2-mili1) / 1000 << " sec. (" << (1000 * n / (mili2-mili1)) << "/sec)" << endl;
#endif

	s.set("hello", "41");
	s.incr("hello");
	s.incr("hello");
	string v;
	s.get("hello", v);

	cout << "v=[" << v << "]" << endl;

	s.shutdown();

	return 0;
}
	
