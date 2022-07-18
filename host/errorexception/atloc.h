
///
/// \file atloc.h
///
/// \brief at location convenience macros for adding file, function and line number when logging or throwing an exception
///
/// \note
/// \li \c need to use std:::string to build this up because __FUNCTION__ in gcc can not be used in preprocessor concatenation
/// \li \c __FUNCTION__ maybe removed from gcc. maybe should use __func__ instead
/// \li \c __LINE__ does not convert correctly if using MSVC edit and continue
///

#ifndef ATLOC_H
#define ATLOC_H

#include <string>

/// \brief line number to a string
#define LINE_STR(xLine) #xLine

/// \brief intermediate level needed to convert line number to a string
#define LINE_CONVERT(xLine) LINE_STR(xLine)


/// \brief add file, function, and line number using c-string 
///
#define FILE_FUNC_LINE  std::string(std::string(__FILE__) + std::string(":") + std::string(__FUNCTION__) + std::string(":") + std::string(LINE_CONVERT(__LINE__))).c_str()

/// \brief use when you want to add file, function, and line number when logging
#define AT_LOC_NO_TAB std::string(std::string("[at ") + std::string(FILE_FUNC_LINE) + std::string("]")).c_str()

/// \brief use if you want to add file, function, and line number when throwing an exception
///
/// \note
/// \li \c the ERROR_EXCEPTION macro already includes this 
#define THROW_LOC_NO_TAB std::string(std::string("[thrown at ") + std::string(FILE_FUNC_LINE) + std::string("]")).c_str()

/// \brief use if you want to add the exception catch location when logging caught exceptions
#define CATCH_LOC_NO_TAB std::string(std::string("[caught at ") + std::string(FILE_FUNC_LINE) + std::string("]")).c_str()


/// \brief use when you want to add file, function, and line number when logging (appends space)
#define AT_LOC std::string(std::string("[at ") + std::string(FILE_FUNC_LINE) + std::string("]   ")).c_str()

/// \brief use if you want to add file, function, and line number when throwing an exception (appends tab)
///
/// \note
/// \li \c the ERROR_EXCEPTION macro already includes this 
#define THROW_LOC std::string(std::string("[thrown at ") + std::string(FILE_FUNC_LINE) + std::string("]    ")).c_str()

/// \brief use if you want to add the exception catch location when logging caught exceptions (appends space)
#define CATCH_LOC std::string(std::string("[caught at ") + std::string(FILE_FUNC_LINE) + std::string("]    ")).c_str()

#endif // ifndef ATLOC_H
