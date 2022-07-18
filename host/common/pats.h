#ifndef _INM__FGREP__H_
#define _INM__FGREP__H_

#include <map>
#include <vector>
#include <list>
#include <iterator>
#include <algorithm>
#include <functional>
#include <string>
#include <cstring>
#include <sstream>


class Pats
{
    typedef std::multimap<char, std::string> Pats_t;
    typedef std::pair<char, std::string> Pats_pair_t;
    typedef Pats_t::const_iterator ConstPatsIter_t;
    typedef Pats_t::iterator PatsIter_t;
    typedef std::pair<ConstPatsIter_t, ConstPatsIter_t> Range_t;

    class EqStr: public std::unary_function<Pats_pair_t, bool>
    {
        const char *m_Cs;
    public:
        explicit EqStr(const char *cs) : m_Cs(cs) { }
        bool operator()(const Pats_pair_t &in) const
        {
            const char *ct = in.second.c_str();
            size_t ctlen = strlen(ct);
            bool beq = (0 == strncmp(m_Cs, ct, ctlen));
            return beq;
        }
    };

    void InsertPat(const char &c, const std::string &pat)
    {
        m_Pats.insert(std::make_pair(c, pat));
    }

public:

    Pats(const std::vector<std::string> &pats)
    {
        std::vector<std::string>::const_iterator iter = pats.begin();
        for ( /* empty */ ; iter != pats.end(); iter++)
        {
            const std::string &pat = (*iter);
            if (!pat.empty())
            {
                const char &c = pat[0];
                InsertPat(c, pat);
            }
        }
    }

    bool IsMatched(const std::string &inputstr, std::string &pat)
    {
        bool bismatched = false;
        size_t len = inputstr.size();
        if (!m_Pats.empty())
        {
            for (size_t i = 0; i < len ; i++)
            {
                const char &c = inputstr[i];
                Range_t range = m_Pats.equal_range(c);
                ConstPatsIter_t iter = find_if(range.first, range.second, 
                                               EqStr(inputstr.c_str() + i));
                bismatched = (iter != range.second);
                if (bismatched)
                {
                    pat = iter->second;
                    break;
                }
            }
        }
 
        return bismatched;
    }

    bool IsMatched(const std::string &inputstr)
    {
        bool bismatched = false;
        size_t len = inputstr.size();
        if (!m_Pats.empty())
        {
            for (size_t i = 0; (i < len) && (false == bismatched); i++)
            {
                const char &c = inputstr[i];
                Range_t range = m_Pats.equal_range(c);
                ConstPatsIter_t iter = find_if(range.first, range.second, 
                                               EqStr(inputstr.c_str() + i));
                bismatched = (iter != range.second);
            }
        }
 
        return bismatched;
    }

private:
    Pats_t m_Pats;
};

#endif /* _INM__FGREP__H_ */
