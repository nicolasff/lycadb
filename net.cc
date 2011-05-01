#include "net.h"
#include "cmd.h"
#include "reply.h"
#include "dispatcher.h"

#include <csignal>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>

#include <event.h>
#include <iostream>
#include <functional> 


using namespace std;
using namespace std::tr1::placeholders; 

Server::Server(string host, short port, Store &store) :
	m_host(host),
	m_port(port) {

	m_dispatcher = new Dispatcher(store);
}

Server::~Server() {
}

void
on_connect(int fd, short event, void *ptr) {

	(void)event;
	Server *s = static_cast<Server*>(ptr);
	
	struct sockaddr_in addr;
	socklen_t addr_sz = sizeof(addr);

	/* accept client */
	int client_fd = accept(fd, (struct sockaddr*)&addr, &addr_sz);
	s->on_connect(client_fd);	// process new client
}

void
Server::reset_event() {
	event_set(&m_ev, m_fd, EV_READ, ::on_connect, this);
	event_base_set(m_base, &m_ev);
	event_add(&m_ev, NULL);
}

void
Server::start() {

	// create server socket
	m_fd = socket();

	// initialize libevent
	m_base = event_base_new();

	// add connection event
	reset_event();
	
	// wait for clients
	event_base_dispatch(m_base);
}

void
Server::on_connect(int fd) {
	
	reset_event(); // reinstall handler for more clients

	Client *c = new Client(fd, this->m_base, *m_dispatcher);
	c->reset_event();
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

	/* this sad list of tests could use a Maybe monad... */

	/* create socket */
	fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == fd) {
		/*syslog(LOG_ERR, "Socket error: %m\n");*/
		return -1;
	}

	/* reuse address if we've bound to it before. */
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
				sizeof(reuse)) < 0) {
		/* syslog(LOG_ERR, "setsockopt error: %m\n"); */
		return -1;
	}

	/* set socket as non-blocking. */
	ret = fcntl(fd, F_SETFD, O_NONBLOCK);
	if (0 != ret) {
		/* syslog(LOG_ERR, "fcntl error: %m\n"); */
		return -1;
	}

	/* bind */
	ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (0 != ret) {
		/* syslog(LOG_ERR, "Bind error: %m\n"); */
		return -1;
	}

	/* listen */
	ret = listen(fd, SOMAXCONN);
	if (0 != ret) {
		/* syslog(LOG_DEBUG, "Listen error: %m\n"); */
		return -1;
	}

	/* there you go, ready to accept! */
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
		delete this;	// ew.
	}
}
