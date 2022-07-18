#include "snapshotmanagerproxy.h"
#include "serializer.h"
#include "xmlmarshal.h"
#include "xmlunmarshal.h"
#include "apinames.h"

int CxSnapshotManager::getSnapshotCount() {
    Serializer sr(m_serializeType);
    const char *api = GET_SNAPSHOT_COUNT;
    if (PHPSerialize == m_serializeType)
    {
        sr.Serialize(m_dpc, api);
    }
    else if (Xmlize == m_serializeType)
    {
        sr.Serialize(api);
    }
    m_transport( sr );
    return sr.UnSerialize<int>();
}
