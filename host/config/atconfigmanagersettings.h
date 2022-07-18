#ifndef FABRICSERVICESETTINGS__H
#define FABRICSERVICESETTINGS__H


#include <list>
#include <string>
#include "svtypes.h"
#include "portablehelpers.h"

enum  {
    FA_STATE_UNDEFINED= 0,
    CREATE_AT_LUN_PENDING = 7,
    CREATE_AT_LUN_DONE = 8,
    DISCOVER_BINDING_DEVICE_PENDING =9,
    DISCOVER_BINDING_DEVICE_DONE = 10,
    DELETE_AT_LUN_PENDING = 13,
    DELETE_AT_LUN_DONE = 14,
    DELETE_AT_LUN_BINDING_DEVICE_PENDING =141,
    DELETE_AT_LUN_BINDING_DEVICE_DONE =142,
    FAILOVER_CX_FILE_CLEANUP_PENDING = 20,
    FAILOVER_DRIVER_DATA_CLEANUP_PENDING = 202,
    FAILOVER_DRIVER_DATA_CLEANUP_DONE = 203
    
    /*
    CREATE_DEVICE_LUN_PENDING = 23,
    CREATE_DEVICE_LUN_DONE = 24,
    DELETE_DEVICE_LUN_PENDING = 25,
    DELETE_DEVICE_LUN_DONE = 26,
    */
};

enum FA_ERROR_STATE_T {
    CREATE_AT_LUN_FAILURE = 2000,
    DELETE_AT_LUN_FAILURE ,
    DISCOVER_BINDING_DEVICE_AI_NOT_FOUND,
    DISCOVER_BINDING_DEVICE_DEVICE_NOT_FOUND,
    DISCOVER_BINDING_DEVICE_OTHER_FAILURE,
    DELETE_AT_LUN_BINDING_DEVICE_FAILURE,
    FAILOVER_DRIVER_DATA_CLEANUP_FAILURE
    /*
    CREATE_DEVICE_LUN_FAILURE,
    DELETE_DEVICE_LUN_FAILURE,
    */
};

/*
	Device LUN states changed to the following:

	2100 - CREATE_DEVICE_LUN_PENDING
	2101 - CREATE_DEVICE_LUN_DONE
	2102 - CREATE_DEVICE_LUN_ID_FAILURE
	2103 - CREATE_DEVICE_LUN_NUMBER_FAILURE

	2110 - DELETE_DEVICE_LUN_PENDING
	2111 - DELETE_DEVICE_LUN_DONE
	2112 - DELETE_DEVICE_LUN_NUMBER_FAILURE
	2113 - DELETE_DEVICE_LUN_ID_FAILURE

	Positive Numbers refer to success.
	Negative Numbers refer to failure.
*/

enum FA_DEVICE_LUN_STATES {
	DEVICE_LUN_INVALID_STATE = 2999,

	CREATE_DEVICE_LUN_PENDING = 2100,
	CREATE_DEVICE_LUN_DONE = 2101,
	CREATE_DEVICE_LUN_ID_FAILURE = 2102,
	CREATE_DEVICE_LUN_NUMBER_FAILURE = 2103,

	DELETE_DEVICE_LUN_PENDING = 2110,
	DELETE_DEVICE_LUN_DONE = 2111,
	DELETE_DEVICE_LUN_NUMBER_FAILURE = 2112,
	DELETE_DEVICE_LUN_ID_FAILURE = 2113
};

#define FA_ERROR_STATE(x) (-x)
struct ApplianceTargetLunInfo
{
    unsigned long long lunno; //as logically lun no can change for each target for the same LUN
    std::string applianceTargetPortWWN;
};

struct ATLunOperations {
    unsigned long long size;
    int blockSize;
    std::string applianceLunId;
    std::list< ApplianceTargetLunInfo > applianceTargetLunInfo;
    std::list< std::string> viListForLunMasking; //All the VI.s which accesses the LUN in that fabric 
    int state; //requested state
    int resultState;	
    ATLunOperations():state(FA_STATE_UNDEFINED) {}
    ATLunOperations(const ATLunOperations & right) : size(right.size),blockSize(right.blockSize), applianceLunId(right.applianceLunId), state(right.state),applianceTargetLunInfo(right.applianceTargetLunInfo), resultState(right.resultState){}

    ATLunOperations& operator=(const ATLunOperations & right) {
        if ( this == &right)
            return *this;
        size = right.size;
	blockSize = right.blockSize;
        applianceLunId = right.applianceLunId;
        state = right.state;
        resultState = right.resultState;
        applianceTargetLunInfo = right.applianceTargetLunInfo;
        return *this;
    }
};

struct ArrayBindingNexus{
    std::string bindingAiPortName;
    std::string bindingVtPortName;
    unsigned int lunNumber;

    bool operator==(const ArrayBindingNexus& right) const 
    {
        return ((bindingAiPortName == right.bindingAiPortName) && (bindingVtPortName == right.bindingVtPortName) && (lunNumber == right.lunNumber) );
    }
};

struct BindingNexus{
    std::string bindingAiPortName;
    std::string bindingVtPortName;
    unsigned long long lunNumber;

    bool operator==(const BindingNexus& right) const {
        return ((bindingAiPortName == right.bindingAiPortName) && (bindingVtPortName == right.bindingVtPortName) && (lunNumber == right.lunNumber) );
         }
    bool operator==(const ArrayBindingNexus& right) const {
        return ((bindingAiPortName == right.bindingAiPortName) && (bindingVtPortName == right.bindingVtPortName) && (lunNumber == right.lunNumber) );
         }

};

typedef std::list<BindingNexus> BindingNexusList;
typedef std::list<ArrayBindingNexus> ArrayBindingNexusList;

struct DiscoverBindingDeviceSetting;
struct ArrayDiscoverBindingDeviceSetting {

    std::string lunId;      // CX will send this, FA should return it
    //AppliaceInitiatorList appliaceInitiatorList;
    ArrayBindingNexusList  bindingNexusList;
    std::string deviceName; // CX leaves blank on get, filled in on success	
    int state;         //requested state 
    int resultState;
    ArrayDiscoverBindingDeviceSetting():state(FA_STATE_UNDEFINED) {}

    ArrayDiscoverBindingDeviceSetting(const ArrayDiscoverBindingDeviceSetting & right) : 
                                    lunId(right.lunId), 
                                    bindingNexusList(right.bindingNexusList),
                                    deviceName(right.deviceName), 
                                    state(right.state),
                                    resultState(right.resultState)  
                                    {}

    ArrayDiscoverBindingDeviceSetting& operator=(const ArrayDiscoverBindingDeviceSetting & right) 
    {
        if ( this == &right)
            return *this;
        lunId = right.lunId;
        bindingNexusList = right.bindingNexusList;
        deviceName = right.deviceName;
        state = right.state;
        resultState = right.resultState;
        return *this;
    }

/*
    ArrayDiscoverBindingDeviceSetting& operator=(const DiscoverBindingDeviceSetting & right) {
        lunId = right.lunId;
        bindingNexusList = right.bindingNexusList;
        deviceName = right.deviceName;
        state = right.state;
        resultState = right.resultState;
        return *this;
    }
*/
};

struct DiscoverBindingDeviceSetting {

    std::string lunId;      // CX will send this, FA should return it
    //AppliaceInitiatorList appliaceInitiatorList;
    BindingNexusList  bindingNexusList;
    std::string deviceName; // CX leaves blank on get, filled in on success	
    bool deviceDelete;
    int state;         //requested state 
    int resultState;
    bool isFirstTimeReq;
    DiscoverBindingDeviceSetting():state(FA_STATE_UNDEFINED),deviceDelete(false), isFirstTimeReq(true) {}

    DiscoverBindingDeviceSetting(const DiscoverBindingDeviceSetting & right) : lunId(right.lunId), deviceName(right.deviceName), 
    bindingNexusList(right.bindingNexusList),deviceDelete(right.deviceDelete),state(right.state),resultState(right.resultState), isFirstTimeReq(right.isFirstTimeReq) {}

    DiscoverBindingDeviceSetting& operator=(const DiscoverBindingDeviceSetting & right) {
        if ( this == &right)
            return *this;
        lunId = right.lunId;
        deviceName = right.deviceName;
        bindingNexusList = right.bindingNexusList;
        deviceDelete = right.deviceDelete;
        state = right.state;
        resultState = right.resultState;
        isFirstTimeReq = right.isFirstTimeReq;
        return *this;
    }
//Convertion routine to convert ArrayBinding... Binding...
    DiscoverBindingDeviceSetting(const ArrayDiscoverBindingDeviceSetting & right) : 
                                    lunId(right.lunId), 
                                    deviceName(right.deviceName), 
                                    deviceDelete(false),
                                    state(right.state),
                                    resultState(right.resultState),  
                                    isFirstTimeReq(false)
                                    {
                                        //bindingNexusList(right.bindingNexusList),
                                        ArrayBindingNexusList::const_iterator iter = right.bindingNexusList.begin();
                                        while (iter != right.bindingNexusList.end())
                                        {
                                            BindingNexus bn;
                                            bn.bindingAiPortName = iter->bindingAiPortName;
                                            bn.bindingVtPortName = iter->bindingVtPortName;
                                            bn.lunNumber = iter->lunNumber;
                                            bindingNexusList.push_back(bn);
                                            iter++;
                                        }

                                    }

    DiscoverBindingDeviceSetting& operator=(const ArrayDiscoverBindingDeviceSetting & right) 
    {
        lunId = right.lunId;
        deviceName = right.deviceName;
        //bindingNexusList = right.bindingNexusList;
        ArrayBindingNexusList::const_iterator iter = right.bindingNexusList.begin();
        while (iter != right.bindingNexusList.end())
        {
            BindingNexus bn;
            bn.bindingAiPortName = iter->bindingAiPortName;
            bn.bindingVtPortName = iter->bindingVtPortName;
            bn.lunNumber = iter->lunNumber;
            bindingNexusList.push_back(bn);
            iter++;
        }

        deviceDelete = false;
        state = right.state;
        resultState = right.resultState;
        isFirstTimeReq = false;
        return *this;
    }
};
typedef std::list<ATLunOperations> ATLunOperationList;
typedef ATLunOperationList::iterator ATLunOperationListIter;
typedef std::list<DiscoverBindingDeviceSetting> DiscoverBindingDeviceSettingList;
typedef DiscoverBindingDeviceSettingList::iterator DiscoverBindingDeviceSettingListIter;

// TODO:below target mode settings also should be retrived as part of FabricAgentInitialSettings



typedef enum  {
    TM_DISABLE = 0,
    TM_ENABLE, 
    TM_UNDEFINED
}TM_OPERATION;

typedef enum {
    TM_OP_START = 0,
    TM_OP_SUCCESS,
    TM_OP_FAILURE,
    //add here
    TM_OP_UNDEFINED = -1
}TM_STATUS;

struct MemberInfo {
    std::string pwwn;	
    std::string nwwn;	
    TM_STATUS tmStatus;
    MemberInfo() {
        pwwn = "";
        nwwn = "";
        tmStatus = TM_OP_UNDEFINED;
    }
    MemberInfo(const MemberInfo & right) : pwwn(right.pwwn), nwwn(right.nwwn), 
    tmStatus(right.tmStatus){}

    MemberInfo& operator=(const MemberInfo & right) {
        if ( this == &right)
            return *this;
        pwwn = right.pwwn;
        nwwn = right.nwwn;
        tmStatus = right.tmStatus;
        return *this;
    }

    bool operator==(const MemberInfo & right) const {
        return ((pwwn == right.pwwn) && (nwwn == right.nwwn) && 
                (tmStatus == right.tmStatus));
    }
};

struct TargetModeOperation {
    TM_OPERATION tmOperation;
    std::list<MemberInfo> tmInfo;

    TargetModeOperation() {
        tmOperation = TM_UNDEFINED;
    }
    TargetModeOperation(const TargetModeOperation &right) {
        tmOperation = right.tmOperation;
        tmInfo.assign(right.tmInfo.begin(), right.tmInfo.end());
    }
    TargetModeOperation& operator=(const TargetModeOperation &right) {
        if ( this == &right)
            return *this;
        tmOperation = right.tmOperation;
        tmInfo.assign(right.tmInfo.begin(), right.tmInfo.end());
        return *this;
    }
    bool operator==(const TargetModeOperation &right) const {
        return ((tmOperation == right.tmOperation) && (tmInfo == right.tmInfo));
    }
};

struct DeviceLunInfo
{
    unsigned long long lunno; //as logically lun no can change for each target for the same LUN
    std::string devicePortWWN;
    std::string deviceID;
};

struct DeviceLunOperations {
    unsigned long long size;
    std::string deviceName;
    std::list< DeviceLunInfo > deviceLunInfo;
    std::list< std::string> viListForLunMasking; //All the VI.s which accesses the LUN in that fabric 
    int state; //requested state
    int resultState;	
    bool flag;
    DeviceLunOperations():state(FA_STATE_UNDEFINED) {}
};
typedef std::list<DeviceLunOperations> DeviceLunOperationList;
typedef DeviceLunOperationList::iterator DeviceLunOperationListIter;

enum { ADD_GROUP = 1, REMOVE_GROUP = 2};
enum { CREATE_ACG_DONE = 3101,  CREATE_ACG_FAILUE = 3300};
enum { DELETE_ACG_DONE = 3201, DELETE_ACG_FAILURE = 3301};
enum { AT_LUN_TYPE = 1, VSNAP_LUN_TYPE = 2};

struct AccessCtrlGroupInfo {
    std::string applianceLundId;
    int LunNo;
    std::list<std::string> initiatorList;
    int operation;    // ADD_GROUP, REMOVE_GROUP
    int state;        // GROUP_OP_SUCCESS, GROUP_OP_FAILURE
    int type;         // AT_LUN_TYPE, VSNAP_LUN_TYPE
    std::string acgid;
    AccessCtrlGroupInfo():state(FA_STATE_UNDEFINED) {}
    AccessCtrlGroupInfo(const AccessCtrlGroupInfo & right) : applianceLundId(right.applianceLundId),LunNo(right.LunNo), initiatorList(right.initiatorList), operation(right.operation),state(right.state), type(right.type), acgid(right.acgid){}

    AccessCtrlGroupInfo& operator=(const AccessCtrlGroupInfo & right) {
        if ( this == &right)
            return *this;
        applianceLundId = right.applianceLundId;
        LunNo = right.LunNo;
        initiatorList = right.initiatorList;
        operation = right.operation;
        state = right.state;
        type = right.type;
        acgid =  right.acgid;
        return *this;
    }
};

struct FabricAgentInitialSettings {	
    ATLunOperationList atLunOperationList;
    DiscoverBindingDeviceSettingList discoverBindingDeviceSettingsList; 
    std::list<TargetModeOperation> atModeConfiguration;
    DeviceLunOperationList deviceLunOperationList;
    std::list<AccessCtrlGroupInfo> groupInfoList;
};


//#ifdef SV_PRISM
enum  {
    DISABLE_APPLIANCE_PASSTHROUGH = 0,
    ENABLE_APPLIANCE_PASSTHROUGH = 1
};
// access control group information
struct AcgInfo {
    unsigned long long lunno; //the lun number presented to each initiator
    std::string groupName;
    std::string sanInitiator;
};

struct PrismATLunOperations {
    unsigned long long size;
    unsigned long long startingPhysicalOffset;
    int blockSize;
    std::string applianceLunId;
    int enableAppliancePassthrough;
    std::string physicalLunPath;
    int state; //requested state
    int resultState;	
    std::list< AcgInfo > acgList;

    PrismATLunOperations():state(FA_STATE_UNDEFINED), enableAppliancePassthrough(DISABLE_APPLIANCE_PASSTHROUGH) {}
    PrismATLunOperations(const PrismATLunOperations & right) : size(right.size), startingPhysicalOffset(right.startingPhysicalOffset), blockSize(right.blockSize), applianceLunId(right.applianceLunId), enableAppliancePassthrough(right.enableAppliancePassthrough), physicalLunPath(right.physicalLunPath), state(right.state), acgList(right.acgList), resultState(right.resultState){}

    PrismATLunOperations& operator=(const PrismATLunOperations & right) {
        if ( this == &right)
            return *this;
        size = right.size;
        startingPhysicalOffset = right.startingPhysicalOffset;
        blockSize = right.blockSize;
        applianceLunId = right.applianceLunId;
        enableAppliancePassthrough = right.enableAppliancePassthrough;
        physicalLunPath = right.physicalLunPath;
        state = right.state;
        resultState = right.resultState;
        acgList = right.acgList;
        return *this;
    }
};

typedef std::list<AcgInfo> AcgInfoList;
typedef AcgInfoList::iterator AcgInfoListIter;
typedef std::list<PrismATLunOperations> PrismATLunOperationList;
typedef PrismATLunOperationList::iterator PrismATLunOperationListIter;

struct PrismAgentInitialSettings {	
    PrismATLunOperationList prismAtLunOperationList;
    DiscoverBindingDeviceSettingList discoverBindingDeviceSettingsList;
};

//#endif SV_PRISM

#endif

