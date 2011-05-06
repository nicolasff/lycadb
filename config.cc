#include "config.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

Config::Config() {

	// default values
	m_options["threads"] = "8";
}

void Config::read(string filename) {

	ifstream f;
	f.open(filename.c_str(), ifstream::in);

	while(f.good()) {
		string l;
		getline(f, l);

		// skip empty lines, comments, and categories.
		if(l.empty() || l[0] == '#' || l[0] == '[') {
			continue;
		}

		// look for key-value pairs separated by spaces and/or tabs.
		size_t sep, valpos;
		if((sep = l.find_first_of(" \t")) == string::npos ||
				(valpos = l.find_first_not_of(" \t", sep)) == string::npos) {
			continue;
		}

		// find the end of the value.
		string::iterator valend = l.end();
		size_t haspos;
		if((haspos = l.find_first_of(" \t#", valpos)) != string::npos) {
			valend = l.begin() + haspos;
		}

		string key, val;
		key.assign(l.begin(), l.begin() + sep);
		val.assign(l.begin() + valpos, valend);

		m_options.insert(make_pair(key, val));
	}

}

Config::const_iterator
Config::begin() const {
	return m_options.begin();
}
Config::const_iterator
Config::end() const {
	return m_options.end();
}

template<>
int
Config::get<int>(string k) const {
	map<string,string>::const_iterator vi;

	if((vi = m_options.find(k)) == m_options.end()) {
		return 1;
	}
	return ::atol(vi->second.c_str());
}
