//define NOMINMAX to avoid ambiguous definitions for min and max
#define NOMINMAX 
#include "ErrorLogger.h"

using namespace ExtendedErrorLogger;

boost::filesystem::path ErrorLogger::s_filepath;
ExtendedErrors ErrorLogger::s_errorList;
boost::function<void(unsigned int logLevel, std::string)> ErrorLogger::s_logCallback;

/// Initializes the json error file name.
/// Should be called once at the start of the program.
void ErrorLogger::Init(const std::string & filepath)
{
    s_filepath = filepath;
    if (boost::filesystem::exists(s_filepath))
    {
        ReadErrorsFromFile(s_errorList);
    }
    s_logCallback = NULL;
}

/// Initializes the json error file name and log callback.
/// Should be called once at the start of the program.
void ErrorLogger::Init(const std::string & filepath,
    boost::function<void(unsigned int, std::string)> callback)
{
    Init(filepath);
    s_logCallback = callback;
}

/// Append one error in json file.
bool ErrorLogger::LogExtendedError(const ExtendedError & extendedError)
{
    try
    {
        s_errorList.Errors.push_back(extendedError);
        WriteErrorsToFile(s_errorList);
    }
    catch (const std::exception & ex)
    {
        Trace(LogLevelError, "Failed to log error with exception " + std::string(ex.what()));
        return false;
    }
    catch (...)
    {
        Trace(LogLevelError, "Failed to log error with generic error");
        return false;
    }
    return true;
}

/// Append multiple errors in json file
bool ErrorLogger::LogExtendedErrorList(const ExtendedErrors & extendedErrorList)
{
    try
    {
        s_errorList.Errors.insert(
            s_errorList.Errors.end(),
            extendedErrorList.Errors.begin(),
            extendedErrorList.Errors.end());
        WriteErrorsToFile(s_errorList);
    }
    catch (const std::exception & ex)
    {
        Trace(LogLevelError, "Failed to log error with exception " + std::string(ex.what()));
        return false;
    }
    catch (...)
    {
        Trace(LogLevelError, "Failed to log error with generic error");
        return false;
    }
    return true;
}

/// Read all the errors from json file.
void ErrorLogger::ReadErrorsFromFile(ExtendedErrors &errorList)
{
    errorList.Errors.clear();

    if(s_filepath.empty())
        throw std::runtime_error("Error json file is not initialized.");

    if (boost::filesystem::exists(s_filepath))
    {
        std::ifstream errorFileIStream(s_filepath.string().c_str(), std::ios::in);
        if (!errorFileIStream.good())
        {
            std::string err = "Failed to open file : " + s_filepath.string();
            throw std::runtime_error(err.c_str());
        }
        std::string errorFileContents((std::istreambuf_iterator<char>(errorFileIStream)),
            std::istreambuf_iterator<char>());
        errorList = JSON::consumer<ExtendedErrors>::convert(errorFileContents, true);
        errorFileIStream.close();
    }
}

/// Write all the errors to json file.
void ErrorLogger::WriteErrorsToFile(ExtendedErrors &errorList)
{
    if (s_filepath.empty())
        throw std::runtime_error("Error json file is not initialized.");

    std::ofstream errorFileOStream(s_filepath.string().c_str());
    if (!errorFileOStream.good())
    {
        std::string err = "Failed to open file : " + s_filepath.string();
        throw std::runtime_error(err.c_str());
    }
    std::string serializedError = JSON::producer<ExtendedErrors>::convert(errorList);
    errorFileOStream << serializedError;
    if (!errorFileOStream.good())
    {
        std::string err = "Failed to write to file : " + s_filepath.string();
        throw std::runtime_error(err.c_str());
    }
    errorFileOStream.close();
}

// Trace the message by calling the callback
void ErrorLogger::Trace(EELogLevel logLevel, const std::string & msg)
{
    if (s_logCallback)
        s_logCallback(logLevel, msg);
}
