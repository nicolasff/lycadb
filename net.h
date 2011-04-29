#ifndef NET_H
#define NET_H

#include <string>

struct event_base;
struct event;

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
	event *m_ev;

	friend void on_connect(int, short, void *);
};

#endif
