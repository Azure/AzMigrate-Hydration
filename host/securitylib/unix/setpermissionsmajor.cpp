#include <sys/stat.h>
#include <fstream>
#include <cerrno>

#include <boost/filesystem.hpp>

#include "errorexception.h"
#include "createpaths.h"

#include "setpermissions.h"
#include "setpermissionsminor.h"

namespace securitylib {

    int defaultFilePermissions()
    {
        return (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    }

    int defaultDirectoryPermissions()
    {
        return (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IXUSR | S_IXGRP);
    }
    
    void setPermissions(std::string const& name, int setFlags)
    {
        if (name.empty())
        {
            throw ERROR_EXCEPTION << "name is blank." << '\n';
        }
        if (!boost::filesystem::exists(name)) {
            if (0 == (SET_PERMISSIONS_NO_CREATE & setFlags)) {
                CreatePaths::createPathsAsNeeded(name);
                if (0 == (SET_PERMISSIONS_NAME_IS_DIR & setFlags)) {
                    std::ofstream oFile(name.c_str());
                } else {
                    boost::filesystem::create_directory(name);
                }
            } else {
                return;
            }
        }

        if (0 != (SET_PERMISSIONS_PARENT_DIR & setFlags)) {
            std::string::size_type idx = name.find_last_of("/\\");
            if (std::string::npos != idx) {
                if ( 0 != chmod(name.substr(0, idx).c_str(), defaultDirectoryPermissions()) ) {
                    throw ERROR_EXCEPTION << "chmod Error: " << strerror(errno) << '\n';
                }
                if (0 != (SET_PERMISSIONS_ALLOW_WEB_SERVER & setFlags)) {
                    setApacheAcl(name.substr(0, idx), setFlags);
                }
            }
        }
        if (0 != (SET_PERMISSIONS_NAME_IS_DIR & setFlags)) {
            if (0 != chmod(name.c_str(),  defaultDirectoryPermissions())) {
                throw ERROR_EXCEPTION << "chmod Error: " << strerror(errno) << '\n';
            }
        } else {
            if (0 != chmod(name.c_str(),  defaultFilePermissions())) {
                throw ERROR_EXCEPTION << "chmod Error: " << strerror(errno) << '\n';
            }
        }
        if (0 != (SET_PERMISSIONS_ALLOW_WEB_SERVER & setFlags)) {
            setApacheAcl(name, setFlags);
        }
    }

    bool setPermissions(const std::string &path, std::string &errstr)
    {
        bool ret = true;

        try {
            setPermissions(path, SET_PERMISSIONS_NO_CREATE);
        }
        catch(ErrorException &ec)
        {
            ret = false;
            errstr = "Exception: ";
            errstr += ec.what();
            errstr += " Failed to set the permissions for file ";
            errstr += path.c_str();
        }
        catch(...)
        {
            ret = false;
            errstr += "Generic exception: Failed to set the permissions for file ";
            errstr += path.c_str();
        }

        return ret;
    }

} // namespace securitylib
