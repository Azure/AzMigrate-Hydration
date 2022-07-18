#ifndef THROTTLEEXCEPTION_H
#define THROTTLEEXCEPTION_H

#include <stdexcept>
#include <string>
#include <sstream>

#include <boost/throw_exception.hpp>
#include <boost/algorithm/string/trim.hpp>

#include "atloc.h"

/// \brief for throwing exceptions
class ThrottlingException : public std::exception {
public:
    /// \brief constructor
    ///
    /// \param data to include with the exception
    explicit ThrottlingException(std::string const& data = std::string())
    {
        m_what = data;
    }

    ThrottlingException(ThrottlingException const& ec)
        : std::exception(ec)
    {
        m_what = ec.m_what;
    }

    /// \brief destructor
    ~ThrottlingException() throw()
    { }

    /// \brief used to add data using iostream style syntax
    template<typename DATA_TYPE>
    ThrottlingException& operator<<(DATA_TYPE const& data) ///< additional data to add
    {
        m_stream << data;
        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }


    /// \brief used to add data using iostream style syntax
    template<typename DATA_TYPE>
    ThrottlingException& operator<<(DATA_TYPE& data) ///< additional data to add
    {
        m_stream << data;
        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }

    /// \brief overload used to add wstring data using iostream style syntax
    ThrottlingException& operator<<(std::wstring const& data) ///< additional data to add
    {
        std::locale loc;

        std::wstring::const_iterator it(data.begin());
        std::wstring::const_iterator endIt(data.end());
        for (/* empty */; it != endIt; ++it) {
            m_stream << std::use_facet<std::ctype<wchar_t> >(loc).narrow(*it, '.');
        }

        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }

    /// \brief overload used to add wstring data using iostream style syntax
    ThrottlingException& operator<<(std::wstring& data) ///< additional data to add
    {
        std::locale loc;

        std::wstring::const_iterator it(data.begin());
        std::wstring::const_iterator endIt(data.end());
        for (/* empty */; it != endIt; ++it) {
            m_stream << std::use_facet<std::ctype<wchar_t> >(loc).narrow(*it, '.');
        }

        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }

    /// \brief overload used to add wchar_t* data using iostream style syntax
    ThrottlingException& operator<<(wchar_t const*& data) ///< additional data to add
    {
        std::locale loc;
        wchar_t const* wCh = data;
        for (/* empty */; L'\0' != *wCh; ++wCh) {
            m_stream << std::use_facet<std::ctype<wchar_t> >(loc).narrow(*wCh, '.');
        }
        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }

    /// \brief overload used to add wchar_t* data using iostream style syntax
    ThrottlingException& operator<<(wchar_t *& data) ///< additional data to add
    {
        std::locale loc;
        wchar_t const* wCh = data;
        for (/* empty */; L'\0' != *wCh; ++wCh) {
            m_stream << std::use_facet<std::ctype<wchar_t> >(loc).narrow(*wCh, '.');
        }
        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }

    /// \brief overload used to add wchar_t data using iostream style syntax
    ThrottlingException& operator<<(wchar_t const& data) ///< additional data to add
    {
        std::locale loc;
        m_stream << std::use_facet<std::ctype<wchar_t> >(loc).narrow(data, '.');
        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }

    /// \brief overload used to add wchar_t data using iostream style syntax
    ThrottlingException& operator<<(wchar_t& data) ///< additional data to add
    {
        std::locale loc;
        m_stream << std::use_facet<std::ctype<wchar_t> >(loc).narrow(data, '.');
        m_what += m_stream.str();
        m_stream.str(std::string());
        m_stream.clear();
        return *this;
    }

    /// \brief get the exception text
    const char* what() const throw() {
        return m_what.c_str();
    }

protected:

private:
    std::stringstream m_stream;
    std::string m_what;
};

#define THROTTLING_EXCEPTION ThrottlingException(AT_LOC)

#endif