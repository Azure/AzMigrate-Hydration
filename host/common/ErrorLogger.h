#ifndef ERROR_LOGGER
#define ERROR_LOGGER

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <limits>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>

#include "json_reader.h"
#include "json_writer.h"

namespace ExtendedErrorLogger
{
    enum EELogLevel
    {
        LogLevelDisable,
        LogLevelFatal,
        LogLevelSevere,
        LogLevelError,
        LogLevelWarning,
        LogLevelInfo,
        LogLevelTrace,
        LogLevelAlways
    };

    class ExtendedError
    {
    public:
        std::string error_name;
        std::map<std::string, std::string> error_params;
        std::string default_message;

        ExtendedError(std::string errorName,
            std::map<std::string, std::string> errorParams,
            std::string defaultMessage)
            : error_name(errorName),
            error_params(errorParams),
            default_message(defaultMessage)
        {}

        ExtendedError()
        {}

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "ExtendedError", false);

            JSON_E(adapter, error_name);
            JSON_KV_E(adapter, "error_params", error_params);
            JSON_T(adapter, default_message);
        }

        void serialize(ptree& node)
        {
            JSON_P(node, error_name);
            JSON_KV_P(node, error_params);
            JSON_P(node, default_message);
        }
    };

    class ExtendedErrors
    {
    public:
        std::vector<ExtendedError> Errors;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "ExtendedErrors", false);

            JSON_T(adapter, Errors);
        }

        void serialize(ptree& node)
        {
            JSON_VCL(node, Errors);
        }
    };

    class ErrorLogger
    {
    public:

        static void Init(const std::string & filepath);

        static void Init(const std::string & filepath, boost::function<void(unsigned int, std::string)> callback);

        static bool LogExtendedError(const ExtendedError & extendedError);

        static bool LogExtendedErrorList(const ExtendedErrors &extendedErrorList);

    private:

        static boost::filesystem::path s_filepath;

        static ExtendedErrors s_errorList;

        static boost::function<void(unsigned int logLevel, std::string)> s_logCallback;

        static void ReadErrorsFromFile(ExtendedErrors &errorList);

        static void WriteErrorsToFile(ExtendedErrors &errorList);

        static void Trace(EELogLevel logLevel, const std::string & msg);
    };
}

#endif // ERROR_LOGGER
