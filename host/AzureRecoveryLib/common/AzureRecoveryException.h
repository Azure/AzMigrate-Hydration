#ifndef AZURE_RECOVERY_EXCEPTION_H
#define AZURE_RECOVERY_EXCEPTION_H

#include <string>
#include <sstream>
#include <stdexcept>

#define THROW_RECVRY_EXCEPTION(x) throw RecoveryException(__FILE__,__LINE__,__FUNCTION__)(x)

#define THROW_CONFIG_EXCEPTION(x) THROW_RECVRY_EXCEPTION(x)

#define THROW_REST_EXCEPTION(x)   THROW_RECVRY_EXCEPTION(x)

class RecoveryException : public std::exception {
public:
    RecoveryException(const char* filename, int line, const char* function)
    {
        std::ostringstream stream;
        stream << filename << '(' << line << ")[" << function << ']';
        m_str = stream.str();
    }

    ~RecoveryException() throw() {}

    template <typename T>
    RecoveryException& operator()(T const& t) {
        std::ostringstream stream;
        stream << " : " << t;
        m_str += stream.str();
        return *this;
    }

    RecoveryException& operator()() { return *this; }

    const char* what() const throw() {
        return m_str.c_str();
    }

private:
    mutable std::string m_str;
};

template<typename TYPE>
inline bool HandleStatusCode(TYPE lStatus, TYPE successCode, TYPE failedCode, const std::string& expceptionMsg)
{
	if (successCode == lStatus) return true;
	else if (failedCode == lStatus) return false;
	else {
		std::stringstream expSteam; expSteam << expceptionMsg << ". Error " << lStatus;
		THROW_RECVRY_EXCEPTION(expSteam.str());
	}
}

#endif // ~AZURE_RECOVERY_EXCEPTION_H