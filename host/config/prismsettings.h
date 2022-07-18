#ifndef PRISM__SETTINGS__H_
#define PRISM__SETTINGS__H_

#include <string>
#include <list>
#include <map>
#include <iterator>
#include "svtypes.h"



/* per source prism setting */
struct PRISM_VOLUME_INFO
{
    enum MIRROR_STATE { MIRROR_STATE_UNKNOWN = -1,
                       MIRROR_SETUP_PENDING = 53, MIRROR_SETUP_COMPLETED = 54, 
                       MIRROR_DELETE_PENDING = 55, MIRROR_SETUP_PENDING_RESYNC_CLEARED = 57 
                      };

    enum VALID_STATUS {
                       SOURCE_LUN_ID_INVALID = 0x01, SOURCE_LUN_CAPACITY_INVALID = 0x02,
                       MIRROR_STATE_INVALID = 0x04, SOURCE_LUN_NAMES_INVALID = 0x08,
                       ATLUN_NUMBER_INVALID = 0x10, ATLUN_NAME_INVALID = 0x20,
                       PI_PWWNS_INVALID = 0x40, AT_PWWNS_INVALID = 0x80
                      };

    typedef enum eMirrorError
    {
        NoError = 0,                     MirrorErrorUnknown = -701, 
        AtlunDiscoveryFailed = -702,     AtlunIdFailed = -703, 
        AgentMemErr = -704,              AtlunDeletionFailed = -705, 
        SrcLunInvalid = -706,            DrvMemAllocErr = -707, 
        DrvMemCopyinErr = -708,          DrvMemCopyoutErr = -709, 
        AtlunInvalid = -710,             SrcNameChangedErr = -711, 
        AtlunNameChangedErr = -712,      MirrorStackingErr = -713,
        ResyncClearError = -714,         ResyncNotSetOnClearErr = -715, 
        SrcDevListMismatchErr = -716,    AtlunDevListMismatchErr = -717, 
        SrcDevScsiIdErr = -718,          AtlunDevScsiIdErr = -719,
        MirrorNotSetup = -720,           MirrorDeleteIoctlFailed = -721,
        MirrorNotSupported = -722,       SrcsTypeMismatch = -723, 
        SrcsIsMulpathMismatch = -724,    SrcsVendorMismatch = -725,
        SrcNotReported = -726,           SrcCapacityInvalid = -727,
        MirrorStateInvalid = -728,       ATLunNumberInvalid = -729,
        ATLunNameInvalid = -730,         PIPwwnsInvalid = -731,
        ATPwwnsInvalid = -732
         
    } MIRROR_ERROR;

    std::string sourceLUNID;                       /* only one ID; key for one source id */
    SV_ULONGLONG sourceLUNCapacity;
    MIRROR_STATE mirrorState;
    std::list<std::string> sourceLUNNames;   
    SV_ULONGLONG applianceTargetLUNNumber;   
    std::string applianceTargetLUNName;            /* name like inmage0000001; should compare after scan and issue inquiry ? */
    std::list<std::string> physicalInitiatorPWWNs;
    std::list<std::string> applianceTargetPWWNs;   
    SV_ULONGLONG sourceLUNStartOffset;

    PRISM_VOLUME_INFO();
    PRISM_VOLUME_INFO(const PRISM_VOLUME_INFO &rhs);
    ~PRISM_VOLUME_INFO();
    PRISM_VOLUME_INFO & operator=(const PRISM_VOLUME_INFO &rhs);
    bool operator==(const PRISM_VOLUME_INFO &rhs) const;
};

/* TODO: should this be map of case less source lun id ? 
 *       need to check */
typedef std::map<std::string, PRISM_VOLUME_INFO> PRISM_SETTINGS; /* key is source lun id */
typedef std::pair<const std::string, PRISM_VOLUME_INFO> PRISM_SETTINGS_PAIR; /* key is source lun id */
typedef PRISM_SETTINGS::const_iterator PRISM_SETTINGS_CONST_ITERATOR;
typedef PRISM_SETTINGS::iterator PRISM_SETTINGS_ITERATOR;

#endif  /* PRISM__SETTINGS__H_ */
