#ifndef STORE_H
#define STORE_H

#include <string>

class Store {

public:
	bool startup();
	bool shutdown();

	bool install();

	bool get(std::string &key, std::string &val);
	bool set(std::string &key, std::string &val);

private:
	bool createDb();
	bool createTable();
};

#endif
