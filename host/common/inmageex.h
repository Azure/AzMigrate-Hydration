//
// inmexception.h: defines an exception that captures the local context
// When we upgrade to VC++ 2005, may use gcc and VC++ vararg macro extensions 
//
#ifndef INMAGEEX_H
#define INMAGEEX_H

#include <stdexcept>
#include <sstream>

#define INMAGE_EX ContextualException( __FILE__, __LINE__, __FUNCTION__ )

class ContextualException : public std::exception {
public:
    ContextualException( const char* filename, int line, const char* function ):
      m_needComma( false )
    {
        std::ostringstream stream;
        stream << filename << '(' << line << ")[" << function << ']';
        m_str = stream.str();
    }

    ~ContextualException() throw() {}

    template <typename T>
    ContextualException& operator()( T const& t ) {
        std::ostringstream stream;
        stream << ( m_needComma ? ',' : ' ' );
        m_needComma = true;
        stream << t;
        m_str += stream.str();
        return *this;
    }

    ContextualException& operator()() { return *this; }

    const char* what() const throw() {
        return m_str.c_str();
    }

private:
    mutable std::string m_str;
    bool m_needComma;
};

inline std::ostream& operator<<( std::ostream& o, ContextualException const& e )
{
	 return o << e.what();
}

#endif // INMAGEEX_H

