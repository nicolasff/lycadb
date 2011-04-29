#ifndef NET_H
#define NET_H

#include <string>
#include <event.h>

#include "protocol.h"

class Client {
public:
	Client(int fd, struct event_base *base);
	void reset_event();

private:
	void on_data();
	void on_cmd(Command *cmd);

	int m_fd;
	event m_ev;
	event_base *m_base;
	Parser m_p;

	friend void on_available_data(int, short, void *);
};

class Server {
public:
	Server(std::string host, short port);
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

	friend void on_connect(int, short, void *);
};

#endif
