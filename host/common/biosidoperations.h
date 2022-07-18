#ifndef BIOS_ID_OPERATIONS_H
#define BIOS_ID_OPERATIONS_H
#include <string>

/*
dmidecode source code :
  * As of version 2.6 of the SMBIOS specification, the first 3
  * fields of the UUID are supposed to be encoded on little-endian.
  * The specification says that this is the defacto standard,
  * however I've seen systems following RFC 4122 instead and use
  * network byte order, so I am reluctant to apply the byte-swapping
  * for older versions.
  *
if (ver >= 0x0206)
printf("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
    p[3], p[2], p[1], p[0], p[5], p[4], p[7], p[6],
    p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
else
printf("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
    p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
    p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
*/

// This function returns the byteswapped bios id according to above rule.
// If input bios id is 420F2F7C-ED56-9C18-7745-C895FF0811F0,
// the output bios id will be 7C2F0F42-56ED-189C-7745-C895FF0811F0

class BiosID
{
public:
    static std::string GetByteswappedBiosID(std::string const & biosid)
    {
        std::string byteswappedBiosId;
        byteswappedBiosId = biosid.substr(6, 2) + biosid.substr(4, 2) + biosid.substr(2, 2) + biosid.substr(0, 2) + "-" + biosid.substr(11, 2) + biosid.substr(9, 2) + "-" + biosid.substr(16, 2) + biosid.substr(14, 2) + "-" + biosid.substr(19, 17);
        return byteswappedBiosId;
    }
};

#endif //BIOS_ID_OPERATIONS_H
