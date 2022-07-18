#include "portablehelpersmajor.h"

#include <set>

#define TEST_FLAG(a, f)     (((a) & (f)) == (f))

class InmageDriverInterface{
private:
    HANDLE      m_hInmageDevice;

public:
    InmageDriverInterface();
    ~InmageDriverInterface();
    bool IsDiskRecoveryRequired();
    void SetSanPolicyToOnlineForAllDisks();
    bool GetDiskIndexList(std::set<SV_ULONG>&  diskIndexList);
};