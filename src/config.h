#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>

class Config {
public:
	Config();
	void read(std::string filename);

	typedef std::map<std::string, std::string>::const_iterator const_iterator;
	const_iterator begin() const;
	const_iterator end() const;

	template<typename T> T get(std::string k) const;
private:
	std::map<std::string, std::string> m_options;
};

#endif
