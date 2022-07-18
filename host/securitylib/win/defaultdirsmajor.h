///
/// \file defaultdirsmajor.h
///
/// \brief
///

#ifndef DEFAULTDIRSMAJOR_H
#define DEFAULTDIRSMAJOR_H

#include <fstream>
#include <stdexcept>
#include <windows.h>

#include <boost/algorithm/string.hpp> 

namespace securitylib {

    std::string const SECURITY_DIR_SEPARATOR("\\");

    inline std::string securityTopDir()
    {
        bool haveProgramData = true;
        std::string topDir;
        std::string systemDrive;

        char* programData = "%ProgramData%";
        std::vector<char> buffer(260);
        size_t size = ExpandEnvironmentStrings(programData, &buffer[0], (DWORD)buffer.size());
        if (size > buffer.size()) {
            buffer.resize(size + 1);
            size = ExpandEnvironmentStrings(programData, &buffer[0], (DWORD)buffer.size());
            if (size > buffer.size()) {
                haveProgramData = false;
            }
        }
        buffer[size] = '\0';
        if (haveProgramData) {
            topDir.assign(&buffer[0]);
        }
        else {
            // best guess
            topDir = "C:\\ProgramData";
        }

        topDir += "\\Microsoft Azure Site Recovery";
        return topDir;
    }

    inline std::string getPSInstallPathFromAppConf(const std::string &confFile)
    {
        std::fstream file(confFile.c_str());
        if (!file)
            throw std::exception(std::string(std::string("Failed to open PS installation app conf file ") + confFile).c_str());

        std::string line, psInstallPath;

        while (getline(file, line))
        {
            if ('#' == line[0] || '[' == line[0] || line.find_first_not_of(' ') == line.npos)
            {
                continue;
            }
            else
            {
                std::string::size_type idx = line.find_first_of("=");
                if (std::string::npos != idx) {
                    if (' ' == line[idx - 1]) {
                        --idx;
                    }
                    if (line.substr(0, idx) == "INSTALLATION_PATH")
                    {
                        if (' ' == line[idx])
                        {
                            ++idx;
                        }
                        psInstallPath = line.substr(idx + 1);
                        boost::trim_if(psInstallPath, boost::is_any_of(" \""));
                    }
                }
            }
        }

        return psInstallPath;
    }

    inline std::string getPSInstallPath(void)
    {
        const char APP_CONF_SUFFIX_PATH[] = "/Config/App.conf";
        std::string app_conf_path = securityTopDir() + APP_CONF_SUFFIX_PATH;
        return getPSInstallPathFromAppConf(app_conf_path);
    }
} // namespace securitylib

#endif // DEFAULTDIRSMAJOR_H