#ifndef NET_H
#define NET_H

#include <string>
#include <vector>
#include <event.h>

#include "protocol.h"

class Dispatcher;

/**
 * A Client object represents a remote TCP client.
 * It maintains it state using a Parser object.
 */
class Client {
public:
	Client(int fd, struct event_base *base, Dispatcher &d);
	void reset_event();

private:
	void on_data();
	void on_cmd(Command *cmd);

	int m_fd;
	event m_ev;
	event_base *m_base;
	Parser m_p;

	Dispatcher &m_dispatcher;

	friend void on_available_data(int, short, void *);
};

class Store;
class Worker;

/**
 * The Server object accepts new connections, creates Client
 * objects, and owns the dispatcher object.
 */
class Server {
public:
	Server(std::string host, short port, int threads, Store &store);
	~Server();
	void start();

private:
	int socket() const;

	void on_connect(int fd);
	void reset_event();

	// server options
	std::string m_host;
	short m_port;

	// network members
	int m_fd;
	event_base *m_base;
	event m_ev;

	// link to store.
	Dispatcher *m_dispatcher;
	Store &m_store;

	friend void on_connect(int, short, void *);

	std::vector<Worker*> m_workers;
	int worker_cur;
};

class Worker {
public:
	Worker(Dispatcher &d);
	void add(int fd);
	void start();

private:
	int m_fd[2];
	event_base *m_base;
	event m_ev;

	pthread_t m_self;

	void read_client();
	void main();

	// link to store.
	Dispatcher &m_dispatcher;

friend	void on_available_client(int fd, short event, void *ptr);
friend	void* on_thread_start(void *);

};

#endif
