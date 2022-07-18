/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: OSVersion.h

Description	: OSVersion structure and its overloaded operators to compare the 
              OS versions, and it also has a static memner function to translate the
			  string representation fo os version numner to OSVersion structure.

History		: 1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_OS_VERSION_H
#define AZURE_RECOVERY_OS_VERSION_H

#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/all.hpp>

#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"

namespace AzureRecovery
{
	struct OSVersion
	{
		int major;
		int minor;

		OSVersion(int _major = 0, int _minor = 0)
		{
			major = _major;
			minor = _minor;
		}

		bool operator == (const OSVersion& rhs) const
		{
			return major == rhs.major &&
				minor == rhs.minor;
		}

		bool operator >= (const OSVersion& rhs) const
		{
			return (major > rhs.major) ||
				(major == rhs.major && minor >= rhs.minor);
		}

		bool operator > (const OSVersion& rhs) const
		{
			return (major > rhs.major) ||
				(major == rhs.major && minor > rhs.minor);
		}

		static OSVersion FromString(const std::string& strVersion)
		{
			TRACE_FUNC_BEGIN;

			std::string str_ver = strVersion;
			boost::trim(str_ver);

			std::vector<std::string> version_nums;

			boost::split(version_nums, str_ver, boost::is_any_of("."));

			if (str_ver.find_first_not_of("0123456789.") != std::string::npos ||
				version_nums.size() != 2 ||
				version_nums[0].empty() ||
				version_nums[1].empty())
			{
				std::string error_msg = strVersion + " value is not in expected format";
				
				TRACE_ERROR("%s\n", error_msg.c_str());

				THROW_RECVRY_EXCEPTION(error_msg);
			}

			TRACE_FUNC_END;

			return OSVersion(boost::lexical_cast<int>(version_nums[0]),
				boost::lexical_cast<int>(version_nums[1]));
		}
	};
}
#endif // ~AZURE_RECOVERY_OS_VERSION_H