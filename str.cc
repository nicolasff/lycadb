#include "str.h"

#include <cstring>

str::str() : m_s(0), m_sz(0) {
}

str::str(const char *s) : m_s(s) {
	m_sz = strlen(s);
}

str::str(const char *s, size_t sz) :
	m_s(s),
	m_sz(sz) {

}
const char *
str::c_str() const {
	return m_s;
}

size_t
str::size() const {
	return m_sz;
}

bool
str::operator!=(const str &s) const {

	if(&s == this) return false;
	return (s.size() != size() || ::memcmp(c_str(), s.c_str(), size()) != 0);
}

bool
str::operator<(const str &s) const {

	if(size() < s.size()) return false;
	return (s.size() == size() && ::memcmp(c_str(), s.c_str(), size()) < 0);
}

bool
str::operator==(const str &s) const {

	if(&s == this) return true;
	return (s.size() == size() && ::memcmp(c_str(), s.c_str(), size()) == 0);
}
