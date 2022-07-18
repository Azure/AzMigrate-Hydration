#ifndef SWITCHINITIALSETTINGS__H
#define SWITCHINITIALSETTINGS__H


#include <list>
#include <map>
#include <string>
#include "svtypes.h"
#include "portablehelpers.h"

using namespace std;

enum  SA_STATES{
	SA_STATE_UNDEFINED= 0,
	CREATE_FR_BINDING_PENDING = 5,
	CREATE_FR_BINDING_DONE = 6,
	DELETE_FR_BINDING_PENDING = 11,
	DELETE_FR_BINDING_DONE = 12,
	DISCOVER_IT_LUNS_PENDING=15,
	DISCOVER_IT_LUNS_DONE=16,
        FRAME_REDIRECT_RESYNC_PENDING = 21,
        FRAME_REDIRECT_RESYNC_ACK = 22,
};

enum  SA_ERROR_STATE_T{
	FRAME_REDIRECT_MESSAGE_SENDING_FAILURE = 1000,
	FRAME_REDIRECT_INVALID_DATA,
	FRAME_REDIRECT_CREATE_AT_LUN_NOT_ACCESSIBLE,
	FRAME_REDIRECT_CREATE_FR_BINDING_CREATE_FAILURE,
	FRAME_REDIRECT_DELETE_FR_BINDING_FAILURE,
        FRAME_REDIRECT_RESYNC_ACK_FAILURE,
	DISCOVER_IT_LUNS_TARGET_FR_CREATE_FAILURE,
	DISCOVER_IT_LUNS_TARGET_NOT_ACCESSIBLE,
	DISCOVER_IT_LUNS_TARGET_LUN_QUERY_FAILURE,
	FRAME_REDIRECT_CREATE_INITIATOR_OFFLINE,
};

#define SA_ERROR_STATE(x) (-x)

struct PortId {
	std::string portWorldWideName;
	std::string nodeWorldWideName;
};

struct LunInfo {
	std::string lunno;	
	std::string size;      
	std::string uuid;	
};

struct ITLinfo {
	PortId realInitiator;
	PortId realTarget;
	LunInfo lunInfo;
	PortId virtualInitiator;
	PortId virtualTarget;
	PortId virtualTargetBinding;
        unsigned int viFcidIndex;
        unsigned int vtFcidIndex;
        unsigned int vtbFcidIndex;
	unsigned int dpcIndex;
	bool restartablePolicy; // true for restartable and false for non restartable;
};

typedef std::list<ITLinfo> ITLInfoList;
typedef ITLInfoList::iterator ITLInfoListIter;


struct ATinfo {
	PortId applianceTarget;
	LunInfo lunInfo;
};

typedef std::list<ATinfo> ATInfoList;
typedef ATInfoList::iterator ATInfoListIter;

struct AIInfo {
	PortId applianceInitiator;
};

typedef std::list<AIInfo> AIInfoList;
typedef AIInfoList::iterator AIInfoListIter;

struct FrameRedirectOperation { 
	std::string physicalLunId;  
	int state;//requested state
        int resultState;//return state
	ITLInfoList itlList;
	ATInfoList atlunList;
	AIInfoList aiList;
	FrameRedirectOperation():state(SA_STATE_UNDEFINED),resultState(SA_STATE_UNDEFINED){}
	~FrameRedirectOperation() {}
};

struct FrameRedirectOperationSetting {
	typedef std::list<FrameRedirectOperation> FrameRedirectOperationList;
	typedef FrameRedirectOperationList::iterator FrameRedirectOperationListIter;
	FrameRedirectOperationList frameRedirectOperationList;
	FrameRedirectOperationSetting() {}
	~FrameRedirectOperationSetting() {}
};

struct LUNInfo
{
	//uint64_t lunNumber;
	//uint64_t capacity;
	SV_ULONGLONG lunNumber;
	SV_ULONGLONG capacity;
	std::string lunUUID;
	std::string vendorId;
	std::string productId;
	std::string productRevision;
	LUNInfo():lunNumber(~0),capacity(0) {}
	~LUNInfo() {}
};
typedef enum {
	PL_DISC_OP_START = 0,
	PL_DISC_OP_SUCCESS,
	PL_DISC_OP_FAILURE,
	//add here
	PL_DISC_OP_UNDEFINED = -1
}PL_DISC_STATUS;

struct PhysicalLunDiscoveryInfo {
	PL_DISC_STATUS pLunStatus;	
	string fabricWWN;
	string initiatorPortName;
	string initiatorNodeName;
	string physicalTargetPortName;
	string physicalTargetNodeName;
	string virtualTargetPortName;
	string virtualTargetNodeName;
	string virtualInitiatorPortName;
	string virtualInitiatorNodeName;
        unsigned int viFcidIndex;
        unsigned int vtFcidIndex;
	bool restartablePolicy;
	//uint8_t dpcNumber;
	SV_UINT dpcNumber;
	std::list<LUNInfo> lunInfo;

	PhysicalLunDiscoveryInfo() {}
	~PhysicalLunDiscoveryInfo() {}
};

struct PhysicalLunDiscoveryInfoList {
	typedef std::list<PhysicalLunDiscoveryInfo> PhysicalLunDiscoveryInfoList_t;
	typedef PhysicalLunDiscoveryInfoList_t::iterator PhysicalLunDiscoveryInfoList_iterator;
	PhysicalLunDiscoveryInfoList_t pLunDiscoveryInfoList;
	PhysicalLunDiscoveryInfoList() {}
	~PhysicalLunDiscoveryInfoList() {}
};

struct SwitchInitialSettings {
	FrameRedirectOperationSetting frameRedirectOperationSet;
	PhysicalLunDiscoveryInfoList physicalLunDiscoveryInfoSet;
	bool enableSlowPathCache;
	SwitchInitialSettings()  {}
	~SwitchInitialSettings() {}
	SwitchInitialSettings(const SwitchInitialSettings&);
	bool operator==( SwitchInitialSettings &) const;
	SwitchInitialSettings& operator=(const SwitchInitialSettings&);

};

typedef std::map<string, string> SwitchAgentInfo;
typedef std::pair<string, string> SwitchAgentInfoPair;

struct SwitchSummary {
	//string guid;
	string switchName;
	string agentVersion;
	//string platformModel;
	//list<string> cpIPAdressList;
	//list<string> prefCpIPAdressList;
	list<string> bpIPAdressList;
	list<string> prefBpIPAdressList;
	//string fabricwwn;
	//string switchwwn;
	//string physicalPortInfo;
	int dpcCount;
	string sasVersion;
	string fosVersion;

	// General Info
	string driverVersion;
	string osInfo;
	int currentCompatibilityNum;
	string installPath;
	OS_VAL osVal;
	string timeZone;
	string localTime;
	string patchHistory;
	string ipAddress;
        string sysVol;
        SV_ULONGLONG sysVolCapacity;
        SV_ULONGLONG sysVolFreeSpace;

	SwitchSummary()  {}
	~SwitchSummary() {}
	SwitchSummary(const SwitchSummary&);
	typedef list<std::string>::iterator bpIpListIter;
	typedef list<std::string>::iterator prefBpIpListLter;
	bool operator==( SwitchSummary &) const;
	SwitchSummary& operator=(const SwitchSummary&);
};
struct PersistSwitchInitialSettings {
	typedef std::pair<string, FrameRedirectOperation> FrameRedirectOperationMapPair;
	typedef std::map<string, FrameRedirectOperation>::iterator FrameRedirectOperationMapIterator;
	std::map<string, FrameRedirectOperation> FrameRedirectOperationMap;
	PersistSwitchInitialSettings() {}
	~PersistSwitchInitialSettings() {}
	PersistSwitchInitialSettings(const PersistSwitchInitialSettings &);
	PersistSwitchInitialSettings(FrameRedirectOperationSetting &);
	//PersistSwitchInitialSettings(std::string serializedstr);
	bool operator==( PersistSwitchInitialSettings &) const;
	PersistSwitchInitialSettings& operator=(const PersistSwitchInitialSettings&);
	operator FrameRedirectOperationSetting ();
	void Add(PersistSwitchInitialSettings &);
};

typedef enum { 
	APPLIANCE_DISK_DOWN = 0, 
	PHYSICAL_DISK_DOWN,
	APPLIANCE_DISK_UP, 
	PHYSICAL_DISK_UP,
	//add here
	DISK_EVENT_UNDEFINED = -1
}EventNotifyType;

struct ITLEventNotifyInfo {
	string iportstr;
	string tportstr;
	string lunno;
	EventNotifyType event;
};
struct ITLEventNotifyInfoSettings {
	typedef std::list<ITLEventNotifyInfo> ITLEventNotifyInfoList_t;
	typedef ITLEventNotifyInfoList_t::iterator ITLEventNotifyInfoList_iterator;
	ITLEventNotifyInfoList_t ITLEventNotifyInfoList;
	ITLEventNotifyInfoSettings() {}
	~ITLEventNotifyInfoSettings() {}

};

struct EventNotifyInfoSettings {
	ITLEventNotifyInfoSettings ITLEventNotifyInfoSet;
	EventNotifyInfoSettings()  {}
	~EventNotifyInfoSettings() {}
	EventNotifyInfoSettings(const EventNotifyInfoSettings&);
	bool operator==( EventNotifyInfoSettings &) const;
	EventNotifyInfoSettings& operator=(const EventNotifyInfoSettings&);
};


typedef enum {
	SA_EXCEPTION_APPLIANCE_DISK_DOWN = 1, 
	SA_EXCEPTION_APPLIANCE_DISK_UP = 0,
    };
typedef enum {
    SA_EXCEPTION_APPLIANCE_LUN_IO_ERROR = 0,
    SA_EXCEPTION_PHYSICAL_LUN_IO_ERROR = 1,
    SA_EXCEPTION_AT_PT_LUN_IO_ERROR = 2,
    SA_EXCEPTION_MIRROR_VOLUME_DISABLE = 3,
    };

typedef enum {
    SA_EXCEPTION_PHYSICAL_DISK_DOWN = 1,
    SA_EXCEPTION_PHYSICAL_DISK_UP = 0,
    };


struct SwitchAgentExceptions
{
    std::string initiatorPortName;
    std::string targetPortName;
    unsigned long long lunNumber;
    int exception;
    int count;
    bool reportedtoCx;
    bool applianceTargetMasked;
    std::string timestamp;   
    SwitchAgentExceptions():count(0),reportedtoCx(false),applianceTargetMasked(false){}
    bool operator==(const SwitchAgentExceptions &rhs) const { return (initiatorPortName == rhs.initiatorPortName && targetPortName == rhs.targetPortName && lunNumber == rhs.lunNumber && exception == rhs.exception ); }
};

typedef std::list<SwitchAgentExceptions> SwitchAgentExceptionsList;


#endif

