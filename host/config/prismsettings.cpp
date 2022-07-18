#include "prismsettings.h"


PRISM_VOLUME_INFO::PRISM_VOLUME_INFO()
{
    mirrorState = MIRROR_STATE_UNKNOWN;
    sourceLUNCapacity = 0;
    /* TODO: the below two are valid values 
     * although based on above two, we 
     * can tell that settings is indeed
     * wrong */
    sourceLUNStartOffset = 0;
    applianceTargetLUNNumber = 0;
}


PRISM_VOLUME_INFO::PRISM_VOLUME_INFO(const PRISM_VOLUME_INFO &rhs)
{
    mirrorState = rhs.mirrorState;
    sourceLUNCapacity = rhs.sourceLUNCapacity;
    sourceLUNStartOffset = rhs.sourceLUNStartOffset;
    sourceLUNID = rhs.sourceLUNID;
    sourceLUNNames = rhs.sourceLUNNames;
    applianceTargetLUNNumber = rhs.applianceTargetLUNNumber;
    applianceTargetLUNName = rhs.applianceTargetLUNName;
    physicalInitiatorPWWNs = rhs.physicalInitiatorPWWNs;
    applianceTargetPWWNs = rhs.applianceTargetPWWNs;
}


PRISM_VOLUME_INFO::~PRISM_VOLUME_INFO()
{
    /* do nothing */
}


PRISM_VOLUME_INFO & PRISM_VOLUME_INFO::operator=(const PRISM_VOLUME_INFO &rhs)
{
    /* TODO: should we check &rhs to be same as this ? Not needed
    if (this == &rhs)
    {
        return *this;
    }
    */

    mirrorState = rhs.mirrorState;
    sourceLUNCapacity = rhs.sourceLUNCapacity;
    sourceLUNStartOffset = rhs.sourceLUNStartOffset;
    sourceLUNID = rhs.sourceLUNID;
    sourceLUNNames = rhs.sourceLUNNames;
    applianceTargetLUNNumber = rhs.applianceTargetLUNNumber;
    applianceTargetLUNName = rhs.applianceTargetLUNName;
    physicalInitiatorPWWNs = rhs.physicalInitiatorPWWNs;
    applianceTargetPWWNs = rhs.applianceTargetPWWNs;

    return *this;
}


bool PRISM_VOLUME_INFO::operator==(const PRISM_VOLUME_INFO &rhs) const
{
    return (
               (mirrorState == rhs.mirrorState) && 
               (sourceLUNCapacity == rhs.sourceLUNCapacity) && 
               (sourceLUNStartOffset == rhs.sourceLUNStartOffset) && 
               (sourceLUNID == rhs.sourceLUNID) &&
               (sourceLUNNames == rhs.sourceLUNNames) &&
               (applianceTargetLUNNumber == rhs.applianceTargetLUNNumber) &&
               (applianceTargetLUNName == rhs.applianceTargetLUNName) &&
               (physicalInitiatorPWWNs == rhs.physicalInitiatorPWWNs) &&
               (applianceTargetPWWNs == rhs.applianceTargetPWWNs)
           );
}


