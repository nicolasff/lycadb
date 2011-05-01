#ifndef STR_H
#define STR_H

#include <cstring> // size_t

class str {

public:
	str();
	str(const char *s);
	str(const char *s, size_t sz, int dup = 0);

	const char *c_str() const;
	size_t size() const;
	void reset();

	bool operator!=(const str &s) const;
	bool operator<(const str &s) const;
	bool operator==(const str &s) const;

private:
	const char *m_s;
	size_t m_sz;
};

#endif
