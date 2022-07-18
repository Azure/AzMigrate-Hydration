#include <fstream>
#include <cerrno>

#ifdef RHEL6
#include <sys/types.h>
#include <sys/acl.h>
#endif

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include "setpermissions.h"
#include "errorexception.h"
#include "scopeguard.h"

namespace securitylib {

#ifdef RHEL6
    struct aclEntrySet {
        acl_tag_t    aclTagType;
        const char *aclQualifier;
        acl_perm_t  aclPerms;
    };

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;

    int getUserOrGroupId(std::string const& name, acl_tag_t aclTagType)
    {
        std::ifstream fileName;
        switch(aclTagType) {
            case ACL_USER:
                fileName.open("/etc/passwd");
                break;
            case ACL_GROUP:
                fileName.open("/etc/group");
                break;
            default:
                return -1;
        }
        
        if (!fileName.good()) {
            return -1;
        }
        boost::char_separator<char> sep(":");
        while (!fileName.eof()) {
            std::string line;
            std::getline(fileName, line);
            if (!line.empty()) {
                if (boost::algorithm::starts_with(line, name)) {
                    tokenizer_t tokens(line, sep);
                    tokenizer_t::iterator iter(tokens.begin());
                    tokenizer_t::iterator iterEnd(tokens.end());
                    // name:*:uid
                    for (int i = 0; i < 2 && iter != iterEnd; ++i, ++iter);
                    if (iter != iterEnd) {
                        return boost::lexical_cast<int>(*iter);
                    }
                    return -1;
                }
            }
        }
        return -1;
    }
    
    const char* getAclTagName(acl_tag_t aclTagType)
    {
        switch(aclTagType) {
            case ACL_USER:
                return "ACL_USER";
            case ACL_GROUP:
                return "ACL_GROUP";
            default:
                return "Unsupported tag";
        }
    }
    
    void setApacheAcl(std::string const& name, int setFlags)
    {
        acl_t acl = acl_get_file(name.c_str(), ACL_TYPE_ACCESS);
        if ((acl_t)NULL == acl)
        {
            throw ERROR_EXCEPTION << "acl_get_file Error: " << strerror(errno) << '\n';
        }
        ON_BLOCK_EXIT(boost::bind<int>(&acl_free, acl));
        
        struct aclEntrySet aclEntries[] = { {ACL_USER, "apache", ((SET_PERMISSIONS_NAME_IS_DIR & setFlags) ? (ACL_READ|ACL_EXECUTE) : ACL_READ)},
                                            {ACL_GROUP, "apache", ((SET_PERMISSIONS_NAME_IS_DIR & setFlags) ? (ACL_READ|ACL_EXECUTE) : ACL_READ)} };

        int aclEntriesCount = (sizeof(aclEntries)/sizeof(aclEntries[0]));
        char *err = NULL;
        for (int i = 0; i < aclEntriesCount; ++i) {
            acl_entry_t aclEntry;
            if ( -1 == acl_create_entry(&acl, &aclEntry) ) {
                err = strerror(errno);
                throw ERROR_EXCEPTION << "acl_create_entry failed for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << ". Error: " << err << '\n';
            }
            
            if ( -1 == acl_set_tag_type(aclEntry, aclEntries[i].aclTagType) ) {
                err = strerror(errno);
                throw ERROR_EXCEPTION << "acl_set_tag_type failed for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << ". Error: " << err << '\n';
            }
            
            id_t id = -1;
            id = getUserOrGroupId(aclEntries[i].aclQualifier, aclEntries[i].aclTagType);
            if (-1 == id) {
                if (SET_PERMISSIONS_IGNORE_WEBUSER_CHK & setFlags) {
                    return; // return silently.
                }
                else {
                    throw ERROR_EXCEPTION << "User/Group for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << " not found." << '\n';
                }
            }
            
            if ( -1 == acl_set_qualifier(aclEntry, (void*)&id) ) {
                err = strerror(errno);
                throw ERROR_EXCEPTION << "acl_set_qualifier failed for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << ". Error: " << err << '\n';
            }
            
            acl_permset_t aclPermset;
            if ( -1 == acl_get_permset(aclEntry, &aclPermset) ) {
                err = strerror(errno);
                throw ERROR_EXCEPTION << "acl_get_permset failed for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << ". Error: " << err << '\n';
            }
            
            if ( -1 == acl_clear_perms(aclPermset) ) {
                err = strerror(errno);
                throw ERROR_EXCEPTION << "acl_clear_perms failed for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << ". Error: " << err << '\n';
            }
            
            if ( -1 == acl_add_perm(aclPermset, aclEntries[i].aclPerms) ) {
                err = strerror(errno);
                throw ERROR_EXCEPTION << "acl_add_perm failed for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << ". Error: " << err << '\n';
            }
            
            if ( -1 == acl_set_permset(aclEntry, aclPermset) ) {
                err = strerror(errno);
                throw ERROR_EXCEPTION << "acl_set_permset failed for "
                                        << getAclTagName(aclEntries[i].aclTagType)
                                        << " "
                                        << aclEntries[i].aclQualifier
                                        << ". Error: " << err << '\n';
            }
        }  // End of for loop

        if ( -1 == acl_calc_mask(&acl) ) {
            err = strerror(errno);
            throw ERROR_EXCEPTION << "acl_calc_mask failed. Error: " << err << '\n';
        }
        
        if ( -1 == acl_set_file(name.c_str(), ACL_TYPE_ACCESS, acl) ) {
            err = strerror(errno);
            throw ERROR_EXCEPTION << "acl_set_file failed. Error: " << err << '\n';
        }
    }
#else
    void setApacheAcl(std::string const& name, int setFlags)
    {
        if (SET_PERMISSIONS_IGNORE_WEBUSER_CHK & setFlags) {
            return; // return silently.
        }
        else {
            throw ERROR_EXCEPTION << "setApacheAcl function is not supported on this OS." << '\n';
        }
    }
#endif

} // namespace securitylib
