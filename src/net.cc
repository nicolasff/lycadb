#include "net.h"
#include "cmd.h"
#include "reply.h"
#include "dispatcher.h"
#include "store.h"

#include <csignal>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>

#include <event.h>
#include <functional>


using namespace std;
using namespace std::tr1::placeholders;

Server::Server(string host, short port, int workers, Store &store) :
	m_host(host),
	m_port(port),
	m_store(store),
	worker_cur(0) {

	m_dispatcher = new Dispatcher(store);

	m_workers.assign(workers, static_cast<Worker*>(0));	// init all workers to zero
}

Server::~Server() {
}

void
on_connect(int fd, short event, void *ptr) {

	(void)event;
	Server *s = static_cast<Server*>(ptr);

	struct sockaddr_in addr;
	socklen_t addr_sz = sizeof(addr);

	// accept client
	int client_fd = accept(fd, (struct sockaddr*)&addr, &addr_sz);
	s->on_connect(client_fd);	// process new client
}

void
Server::reset_event() {
	event_set(&m_ev, m_fd, EV_READ, ::on_connect, this);
	event_base_set(m_base, &m_ev);
	event_add(&m_ev, NULL);
}

static event_base *base;
static void
on_sigint(int) {
	event_base_loopbreak(base);
}

void
Server::start() {

	signal(SIGINT, on_sigint);

	// create server socket
	m_fd = socket();

	// initialize libevent
	base = m_base = event_base_new();

	// add connection event
	reset_event();

	// start threads
	vector<Worker*>::iterator wi;
	for(size_t i = 0; i < m_workers.size(); ++i) {
		Worker *w = new Worker(*m_dispatcher);
		m_workers[i] = w;
		w->start();
	}

	// wait for clients
	event_base_dispatch(m_base);

	// cleanup after shutdown request
	m_store.shutdown();
}

void
Server::on_connect(int fd) {

	reset_event(); // reinstall handler for more clients

	m_workers[worker_cur]->add(fd);
	worker_cur = (worker_cur+1) % m_workers.size();
}


/**
 * Sets up a non-blocking socket
 */
int
Server::socket() const {

	int reuse = 1;
	struct sockaddr_in addr;
	int fd, ret;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);

	memset(&(addr.sin_addr), 0, sizeof(addr.sin_addr));
	addr.sin_addr.s_addr = inet_addr(m_host.c_str());

	// this sad list of tests could use a Maybe monad...

	// create socket
	fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == fd) {
		//syslog(LOG_ERR, "Socket error: %m\n");
		return -1;
	}

	// reuse address if we've bound to it before.
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
				sizeof(reuse)) < 0) {
		// syslog(LOG_ERR, "setsockopt error: %m\n");
		return -1;
	}

	// set socket as non-blocking.
	ret = fcntl(fd, F_SETFD, O_NONBLOCK);
	if (0 != ret) {
		// syslog(LOG_ERR, "fcntl error: %m\n");
		return -1;
	}

	// bind
	ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (0 != ret) {
		// syslog(LOG_ERR, "Bind error: %m\n");
		return -1;
	}

	// listen
	ret = listen(fd, SOMAXCONN);
	if (0 != ret) {
		// syslog(LOG_DEBUG, "Listen error: %m\n");
		return -1;
	}

	// there you go, ready to accept!
	return fd;
}

////////////////////////////////////////////////////////////////////////////////

Client::Client(int fd, struct event_base *base, Dispatcher &d) :
	m_fd(fd),
	m_base(base),
	m_dispatcher(d)
{
	// setup parser success callback
	m_p.setSuccessCb(tr1::bind(&Client::on_cmd, this, _1));

}

void
Client::on_cmd(Command *cmd) {

	Reply *r = m_dispatcher.run(*cmd);
	r->write(m_fd);

	delete r;
	delete cmd;
}

void
on_available_data(int fd, short event, void *ptr) {

	(void)event;
	(void)fd;

	Client *c = static_cast<Client*>(ptr);
	c->on_data();	// process new data
}

void
Client::reset_event() {
	event_set(&m_ev, m_fd, EV_READ, ::on_available_data, this);
	event_base_set(m_base, &m_ev);
	event_add(&m_ev, NULL);
}

void
Client::on_data() {

	char buffer[4096];
	int ret = read(m_fd, buffer, sizeof(buffer));

	if(ret > 0) {
		reset_event();	// reinstall event
		m_p.consume(buffer, (size_t)ret);
	} else {
		close(m_fd);
		delete this;	// ew.
	}
}

////////////////////////////////////////////////////////////////////////////////

void*
on_thread_start(void *ptr) {
	Worker *w = static_cast<Worker*>(ptr);

	w->main();

	return 0;
}

void
on_available_client(int fd, short event, void *ptr) {

	(void)event;
	(void)fd;

	Worker *w = static_cast<Worker*>(ptr);
	w->read_client();	// process new data
}

Worker::Worker(Dispatcher &d):
	m_dispatcher(d) {

	m_base = event_base_new();
	int ret = pipe(m_fd);
	(void)ret;

	event_set(&m_ev, m_fd[0], EV_READ | EV_PERSIST, ::on_available_client, this);
	event_base_set(m_base, &m_ev);
	event_add(&m_ev, NULL);
}

void
Worker::start() {

	pthread_create(&m_self, NULL, on_thread_start, this);
}

void
Worker::main() {

	event_base_dispatch(m_base);
}

void
Worker::add(int fd) {
	int ret = write(m_fd[1], &fd, sizeof(fd));
	(void)ret;
}

void
Worker::read_client() {
	int fd;
	int ret = read(m_fd[0], &fd, sizeof(fd));
	(void)ret;

	Client *c = new Client(fd, this->m_base, m_dispatcher);
	c->reset_event();
}
