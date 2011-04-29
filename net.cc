#include "net.h"

#include <csignal>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>

#include <event.h>
#include <iostream>


using namespace std;

Server::Server(string host, short port) :
	m_host(host),
	m_port(port) {

	m_ev = new event;
}

Server::~Server() {
	delete m_ev;
}

void
on_connect(int fd, short event, void *ptr) {

	(void)event;
	Server *s = static_cast<Server*>(ptr);
	
	s->on_connect(fd);	// process new client
}

void
Server::reset_event() {
	event_set(m_ev, m_fd, EV_READ, ::on_connect, this);
	event_base_set(m_base, m_ev);
	event_add(m_ev, NULL);
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

	cout << fd << " connected." << endl;
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
