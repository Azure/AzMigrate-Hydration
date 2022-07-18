#ifndef _INMSTRCMP__H_
#define _INMSTRCMP__H_

#include <string>
#include <cstring>
#include <cctype>

class CharCmp
{
public:
    static bool eq(const char a, const char b)
    {
        return a == b;
    }
    static bool lt(const char a, const char b)
    {
        return a < b;
    }
};

class NoCaseCharCmp
{
public:
    static bool eq(const char a, const char b)
    {
        return tolower(a) == tolower(b);
    }
    static bool lt(const char a, const char b)
    {
        return tolower(a) < tolower(b);
    }
};

template<class C>
int InmStrCmp(const std::string &s, const std::string &t)
{
    for (size_t i = 0; (i < s.length()) && (i < t.length()); i++)
    {
        if (!C::eq(s[i], t[i]))
        {
            return C::lt(s[i], t[i]) ? -1 : 1;
        }
    }

    return s.length() - t.length();
}

class InmLessNoCase
{
public:
    bool operator()(const std::string &s, const std::string &t) const
    {
        return InmStrCmp<NoCaseCharCmp>(s, t) < 0;
    }
};

class InmCStringLess
{
public:
    bool operator()(const char *p, const char *q) const
    {
        return (strcmp(p, q) < 0);
    }
};


inline bool EndsWith(const std::string &str, const std::string &pattern)
{
    std::string::size_type idx = str.rfind(pattern);
    return (std::string::npos != idx) && ((idx + pattern.length()) == str.length());
}

#endif

