#include "str.h"

#include <cstring>

str::str() : m_s(0), m_sz(0) {
}

str::str(const char *s) : m_s(s) {
	m_sz = strlen(s);
}

str::str(const char *s, size_t sz, int dup) :
	m_s(s),
	m_sz(sz) {

	if(dup) {
		m_s = new char[sz+1];
		memcpy((char*)m_s, (const char*)s, sz);
		((char*)m_s)[sz] = 0;	// end with null char if dup'd
	}

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
str::empty() const {
	return (m_sz == 0);
}

void
str::reset() {
	delete[] m_s;
	m_sz = 0;
}

bool
str::operator!=(const str &s) const {

	if(&s == this) return false;
	return (s.size() != size() || ::memcmp(c_str(), s.c_str(), size()) != 0);
}

bool
str::operator<(const str &s) const {

	size_t min_sz = m_sz < s.size() ? m_sz : s.size();
	return (::memcmp(c_str(), s.c_str(), min_sz) < 0);
}

bool
str::operator==(const str &s) const {

	if(&s == this) return true;
	return (s.size() == size() && ::memcmp(c_str(), s.c_str(), size()) == 0);
}
