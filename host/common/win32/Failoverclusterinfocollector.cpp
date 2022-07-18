/*
    FailoverClusterInfoCollector.cpp: define Collector for the FAILOVER_CLUSTER_INFO_COLLECTOR
*/

#include "Failoverclusterinfocollector.h"
#include "volumeinfocollector.h"
#include "registry.h"
#include <atlbase.h>

using namespace std;

bool FailoverClusterInfo::IsClusterNode()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    bool bRet = false;
    FailoverCluster::ClusterServiceStatus status(FailoverCluster::ClusterServiceStatus::ClusterStateUnknown);
    
    if (GetClusSvcStatusOnCurrentNode(status))
    {
        if (status == FailoverCluster::ClusterServiceStatus::ClusterStateRunning)
        {
            bRet = true;
        }
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return bRet;
}

bool FailoverClusterInfo::GetClusSvcStatusOnCurrentNode(FailoverCluster::ClusterServiceStatus& status)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    stringstream err_msg;
    bool bRet = false;

    DWORD state = FailoverCluster::ClusterServiceStatus::ClusterStateUnknown;
    DWORD err = GetNodeClusterState(NULL, &state);

    if (err != ERROR_SUCCESS)
    {
        err_msg << "GetNodeClusterState operation failed with system error code: " << err;
        m_ErroMessage = err_msg.str();

        return bRet;
    }

    if (state == NODE_CLUSTER_STATE::ClusterStateNotInstalled)
    {
        status = FailoverCluster::ClusterServiceStatus::ClusterStateNotInstalled;
        err_msg << "Node Cluster State: The Cluster service is not installed on the node";
    }
    else if (state == NODE_CLUSTER_STATE::ClusterStateNotConfigured)
    {
        status = FailoverCluster::ClusterServiceStatus::ClusterStateNotConfigured;
        err_msg << "Node Cluster State: The Cluster service is installed on the node but has not yet been configured";
    }
    else if (state == ClusterStateNotRunning)
    {
        status = FailoverCluster::ClusterServiceStatus::ClusterStateNotRunning;
        err_msg << "Node Cluster State: The Cluster service is installed and configured on the node but is not currently running";
    }
    else if (state == ClusterStateRunning)
    {
        status = FailoverCluster::ClusterServiceStatus::ClusterStateRunning;
        DebugPrintf(SV_LOG_ALWAYS, "%s: Node Cluster State: The Cluster service is installed, configured, and running on the node\n", FUNCTION_NAME);
        
        bRet = true;
    }
    else
    {
        status = FailoverCluster::ClusterServiceStatus::ClusterStateUnknown;
        err_msg << "Node Cluster State: The Cluster service state is unknown";
    }

    if (!err_msg.str().empty())
    {
        m_ErroMessage = err_msg.str();
        DebugPrintf(SV_LOG_ERROR, "%s: %s.\n", FUNCTION_NAME, m_ErroMessage.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return bRet;
}

std::string FailoverClusterInfo::GetLastErrorMessage(void)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return m_ErroMessage;
}

void FailoverClusterInfo::AddFailoverClusterProperty(const std::string& key, const std::string& value)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    DebugPrintf(SV_LOG_DEBUG, "%s: Adding key %s with value %s in failover cluster properties.\n", FUNCTION_NAME, key.c_str(), value.c_str());
    m_Properties[key] = value;

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

}

std::string FailoverClusterInfo::GetFailoverClusterProperty(const std::string& key)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    std::string retVal;

    std::map<std::string, std::string>::iterator iter = m_Properties.find(key);
    if (iter == m_Properties.end())
    {
        DebugPrintf(SV_LOG_ERROR, "%s: Key %s is not present in properties.\n", FUNCTION_NAME, key.c_str());
    }
    else
    {
        retVal = iter->second;
        DebugPrintf(SV_LOG_DEBUG, "%s: Key %s is present in properties and its values is %s\n", FUNCTION_NAME, key.c_str(), retVal.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return retVal;
}

SVSTATUS FailoverClusterInfo::CollectFailoverClusterProperties()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    SVSTATUS status = CollectFailoverClusterProperties(false);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return status;
}

SVSTATUS FailoverClusterInfo::CollectFailoverClusterProperties(bool collectClusterDiskInformationAlso)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    if (GetFailoverClusterName() != SVS_OK)
    {
        return SVE_ABORT;
    }

    if (GetClusterId(std::string()) != SVS_OK)
    {
        return SVE_ABORT;
    }

    if (GetFailoverClusterNodesInfo() !=  SVS_OK)
    {
        return SVE_ABORT;
    }

    if (GetCurrentClusterNodesInfo() != SVS_OK)
    {
        return SVE_ABORT;
    }

    if (GetClusterResources() != SVS_OK)
    {
        return SVE_ABORT;
    }

    if (collectClusterDiskInformationAlso)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s: Collecting cluster disk information\n", FUNCTION_NAME);

        VolumeInfoCollector volumeCollector;
        volumeCollector.GetVolumeInfos(m_volumeSummaries, m_volumeDynamicInfos, true, true);
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return SVS_OK;
}

SVSTATUS FailoverClusterInfo::GetFailoverClusterName()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    SVSTATUS status = SVS_OK;

    HCLUSTER hcluster;

    std::vector<WCHAR> lpszClusterName(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');
    
    hcluster = OpenCluster(
        NULL
    );

    stringstream err_msg;
    if (!hcluster) {

        err_msg << "Failed to open a handle to cluster with error code: " << GetLastError();
        m_ErroMessage = err_msg.str();

        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());

        return SVE_FAIL;
    }
    DebugPrintf(SV_LOG_DEBUG, "%s: Opened handle to cluster\n", FUNCTION_NAME);

    DWORD cchNameSize = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

    DWORD dwResult;

    bool bRetry;

    do{
        bRetry = false;
        
        m_ClusterVersionInfo.dwVersionInfoSize = sizeof(CLUSTERVERSIONINFO);

        dwResult = GetClusterInformation(
            hcluster,
            &lpszClusterName[0],
            &cchNameSize,
            &m_ClusterVersionInfo
        );

        if (dwResult == ERROR_MORE_DATA)
        {
            DebugPrintf(SV_LOG_DEBUG, "%s: ERROR_MORE_DATA observed, incresing the size of buffer to %d\n", FUNCTION_NAME, cchNameSize);
            bRetry = true;
            
            cchNameSize++;
            lpszClusterName.resize(cchNameSize, L'\0');

            continue;
        }

        if (dwResult == ERROR_SUCCESS)
        {
            std::string clusterName = CW2A(&lpszClusterName[0]);
            AddFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_NAME, clusterName);
            
            status = SVS_OK;
            
        }
        else
        {
            err_msg << "GetClusterInformation failed with error code: " << dwResult;
            m_ErroMessage = err_msg.str();

            DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());

            status = SVE_FAIL;
            
        }

    } while (bRetry);

    CloseCluster(hcluster);
    
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return status;
}


SVSTATUS FailoverClusterInfo::GetFailoverClusterNodesInfo()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
  
    SVSTATUS status = SVS_OK;
    stringstream err_msg;

    std::vector<WCHAR> lpszClusterName(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');
    HCLUSTER hcluster;

    hcluster = OpenCluster(
        NULL
    );

    if (!hcluster) {

        err_msg << "Failed to open a handle to cluster with error code: " << GetLastError();
        m_ErroMessage = err_msg.str();
        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, err_msg.str().c_str());
        
        return SVE_FAIL;

    }
    DebugPrintf(SV_LOG_DEBUG, "%s: Opened handle to cluster\n", FUNCTION_NAME);

    DWORD dwType = CLUSTER_ENUM_NODE;
    DWORD cchNameSize = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

    HCLUSENUM hClusEnum;

    hClusEnum = ClusterOpenEnum(
        hcluster,
        dwType
    );

    if (!hClusEnum) {
        err_msg << "Failed to open a handle to cluster with error code: " << GetLastError();
        m_ErroMessage = err_msg.str();
        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, err_msg.str().c_str());
        status = SVE_FAIL;

        return status;
    }

    DebugPrintf(SV_LOG_DEBUG, "%s: Opened a handle to cluster enum of type node\n", FUNCTION_NAME);

    DWORD cbItem = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwIndex = 0;
    std::vector<WCHAR> lpszName(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');

    std::set<NodeEntity> s_ClusterNodes;

    bool bRetry = false;
    do{

        bRetry = false;
        
        dwResult = ClusterEnum(
            hClusEnum,
            dwIndex,
            &dwType,
            &lpszName[0],
            &cbItem
        );

        if (dwResult == ERROR_NO_MORE_ITEMS)
        {
            DebugPrintf(SV_LOG_DEBUG, "%s: no more items present in cluster node enumeration\n", FUNCTION_NAME);
            break;
        }

        if (dwResult == ERROR_MORE_DATA) {
            
            cbItem++;
            lpszName.resize(cbItem, L'\0');
            bRetry = true;

            continue;
        }

        if (dwResult == ERROR_SUCCESS) {

            HNODE hnode = NULL;
            NodeEntity node;

            node.nodeName = CW2A(&lpszName[0]);

            hnode = OpenClusterNode(
                hcluster,
                &lpszName[0]
            );
            
            if (!hnode) {

                err_msg << "Failed to opne a handle to cluster node: " <<node.nodeName << " with error: " << GetLastError();
                m_ErroMessage = err_msg.str();
                DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());

                return SVE_FAIL;
            }

            
            DebugPrintf(SV_LOG_DEBUG, "%s: Opened a handle to clusternode: %s\n", FUNCTION_NAME, node.nodeName.c_str());

            DWORD state = GetClusterNodeState(
                hnode
            );
            node.nodeState = FailoverCluster::CLUSTER_NODE_STATE(state);

            std::vector<WCHAR> lpszNodeId(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');
            DWORD cbNodeId = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

            bool bRetryForId = false;

            do {
                bRetryForId = false;

                DWORD dwResultNodeId = GetClusterNodeId(
                    hnode,
                    &lpszNodeId[0],
                    &cbNodeId
                );

                if (dwResultNodeId == ERROR_MORE_DATA)
                {
                    cbNodeId++;
                    DebugPrintf(SV_LOG_DEBUG, "%s: ERROR_MORE_DATA observed while getting the ClusterNode Id for the node: %s, resizing the size to: %d\n"
                        , FUNCTION_NAME, node.nodeName.c_str(), cbNodeId);

                    bRetryForId = true;

                    continue;

                }

                node.nodeId = CW2A(&lpszNodeId[0]);
                DebugPrintf(SV_LOG_DEBUG, "%s: Inserting node with node name: %s, node id: %s, node state: %d to node list\n", FUNCTION_NAME, node.nodeName.c_str(), node.nodeId.c_str(), node.nodeState);
                
                s_ClusterNodes.insert(node);

            } while (bRetryForId);

            CloseClusterNode(hnode);
        }
        
        dwIndex++;

    } while (bRetry || dwResult == ERROR_SUCCESS);

    ClusterCloseEnum(hClusEnum);
    CloseCluster(hcluster);

    {
        m_ClusterNodes = s_ClusterNodes;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return status;
}

SVSTATUS FailoverClusterInfo::GetClusterResources()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    SVSTATUS status = SVS_OK;
    stringstream err_msg;

    std::vector<WCHAR> lpszClusterName(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');
    
    HCLUSTER hcluster;

    hcluster = OpenCluster(
        NULL
    );

    if (!hcluster) {

        err_msg << "Failed to open a handle to cluster with error code: " << GetLastError();
        m_ErroMessage = err_msg.str();
        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, err_msg.str().c_str());
        status = SVE_FAIL;

        return status;

    }
    DebugPrintf(SV_LOG_DEBUG, "%s: Opened handle to cluster\n", FUNCTION_NAME);

    DWORD dwType = CLUSTER_ENUM_RESOURCE;
    DWORD cchNameSize = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

    HCLUSENUM hClusEnum;

    hClusEnum = ClusterOpenEnum(
        hcluster,
        dwType
    );

    if (!hClusEnum) {
        err_msg << "Failed to open a handle to cluster with error code: " << GetLastError();
        m_ErroMessage = err_msg.str();
        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, err_msg.str().c_str());
        status = SVE_FAIL;

        return status;
    }

    DebugPrintf(SV_LOG_DEBUG, "%s: Opened a handle to cluster enum of type resource\n", FUNCTION_NAME);

    DWORD cbItem = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;
    DWORD dwResult = ERROR_SUCCESS;
    DWORD dwIndex = 0;

    std::vector<WCHAR> lpszResourceName(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');

    std::set<ResourceEntity> s_ClusterResources;

    bool bRetry = false;
    cbItem = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

    do{
        bRetry = false;

        dwResult = ClusterEnum(
            hClusEnum,
            dwIndex,
            &dwType,
            &lpszResourceName[0],
            &cbItem
        );

        if (dwResult == ERROR_NO_MORE_ITEMS)
        {
            DebugPrintf(SV_LOG_DEBUG, "%s: no more items present in cluster node enumeration\n", FUNCTION_NAME);
            break;
        }

        if (dwResult == ERROR_MORE_DATA) {

            cbItem++;

            DebugPrintf(SV_LOG_DEBUG, "%s: ERROR_MORE_DATA observed during resource enum\n", FUNCTION_NAME);
            lpszResourceName.resize(cbItem, L'\0');

            bRetry = true;

            continue;
        }
        
        if (dwResult == ERROR_SUCCESS) {

            ResourceEntity resourceEntity;
            std::string resourceName = CW2A(&lpszResourceName[0]);
            if (GetReosurceInfo(hcluster, resourceName, resourceEntity) != SVS_OK)
            {
                DebugPrintf(SV_LOG_ERROR, "%s: GetReosurceInfo operation failed with error: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());
                return SVE_FAIL;
            }

            DebugPrintf(SV_LOG_DEBUG, "%s: Inserting resource entity with resource id: %s, resource name: %s, resource type: %s\n", FUNCTION_NAME,
                resourceEntity.resourceId.c_str(), resourceEntity.resourceName.c_str(), resourceEntity.resourceType.c_str());

            s_ClusterResources.insert(resourceEntity);

        }

        dwIndex++;
    
    } while (bRetry || dwResult == ERROR_SUCCESS);

    ClusterCloseEnum(hClusEnum);
    CloseCluster(hcluster);

    {
        m_ClusterResources = s_ClusterResources;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return status;
}


SVSTATUS FailoverClusterInfo::GetReosurceInfo(HCLUSTER hcluster, std::string& resourceName, ResourceEntity& entity)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    SVSTATUS status = SVS_OK;
    stringstream err_msg;

    HRESOURCE hResource = NULL;

    entity.resourceName = resourceName;
    
    std::vector<WCHAR> lpszResourceName;
    
    std::copy(resourceName.begin(), resourceName.end(), std::back_inserter(lpszResourceName));
    lpszResourceName.push_back(L'\0');

    hResource = OpenClusterResource(
        hcluster,
        &lpszResourceName[0]
    );

    if (!hResource) {

        err_msg << "Failed to opne a handle to cluster resource: " << entity.resourceName << " with error: " << GetLastError();
        m_ErroMessage = err_msg.str();
        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());

        return SVE_FAIL;
    }

    DebugPrintf(SV_LOG_DEBUG, "%s: Opened a handle to cluster resource: %s\n", FUNCTION_NAME, entity.resourceName.c_str());

    DWORD cbTypeBuffer = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;
    DWORD cbBytesReturned = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

    DWORD dwRetVal = ERROR_SUCCESS;
    std::vector<WCHAR> lpTypeBuffer(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');
    
    bool bRetry = false;
    
    do {

        bRetry = false;

        dwRetVal = ClusterResourceControl(
            hResource,
            NULL,
            CLUSCTL_RESOURCE_GET_RESOURCE_TYPE,
            NULL,
            NULL,
            &lpTypeBuffer[0],
            cbTypeBuffer,
            &cbBytesReturned
        );

        if (dwRetVal == ERROR_MORE_DATA)
        {
            cbTypeBuffer = cbBytesReturned + 1;
            lpTypeBuffer.resize(cbTypeBuffer, L'\0');

            bRetry = true;

            continue;
        }

        if (dwRetVal != ERROR_SUCCESS)
        {
            err_msg << "ClusterResourceControl CLUSCTL_RESOURCE_GET_RESOURCE_TYPE failed for the resource: " << entity.resourceName << " with error code: " << GetLastError();
            m_ErroMessage = err_msg.str();
            DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());
            
            return SVE_FAIL;
        }

        entity.resourceType = CW2A(&lpTypeBuffer[0]);
        
    } while (bRetry);

    DWORD cbIdBuffer = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;
    cbBytesReturned = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

    dwRetVal = ERROR_SUCCESS;
    std::vector<WCHAR> lpIdBuffer(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');

    do {

        bRetry = false;

        dwRetVal = ClusterResourceControl(
            hResource,
            NULL,
            CLUSCTL_RESOURCE_GET_ID,
            NULL,
            NULL,
            &lpIdBuffer[0],
            cbIdBuffer,
            &cbBytesReturned
        );

        if (dwRetVal == ERROR_MORE_DATA)
        {
            cbIdBuffer = cbBytesReturned + 1;
            lpIdBuffer.resize(cbBytesReturned, L'\0');

            bRetry = true;

            continue;
        }

        if (dwRetVal != ERROR_SUCCESS)
        {
            err_msg << "ClusterResourceControl CLUSCTL_RESOURCE_GET_ID failed for the resource: " << entity.resourceName << " with error code: " << GetLastError();
            m_ErroMessage = err_msg.str();
            DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());
            
            return SVE_FAIL;
        }

        entity.resourceId = CW2A(&lpIdBuffer[0]);

    } while (bRetry);

    CloseClusterResource(hResource);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return status;
}


SVSTATUS FailoverClusterInfo::GetCurrentClusterNodesInfo()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVSTATUS status = SVS_OK;

    stringstream err_msg;

    std::vector<WCHAR> lpszNodeId(FAILOVER_CLUSTER_GENERIC_BUFFER_LEN, L'\0');
    DWORD lpcbReturned = FAILOVER_CLUSTER_GENERIC_BUFFER_LEN;

    if (GetCurrentClusterNodeId(&lpszNodeId[0], &lpcbReturned) == ERROR_MORE_DATA)
    {
        lpcbReturned++;
        if (GetCurrentClusterNodeId(&lpszNodeId[0], &lpcbReturned) != ERROR_SUCCESS)
        {
            err_msg << " GetCurrentClusterNodeId() failed with the error: " << GetLastError();
            m_ErroMessage = err_msg.str().c_str();
            DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());
            status = SVE_FAIL;

            return status;
        }
    }

    std::string currentNodeId = CW2A(&lpszNodeId[0]);

    DebugPrintf(SV_LOG_DEBUG, "%s: current node id is: %s\n", FUNCTION_NAME, currentNodeId.c_str());
    AddFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_ID, currentNodeId);

    std::set<NodeEntity>::iterator iter = m_ClusterNodes.begin();
    std::string currentNodeName;
    
    for (; iter != m_ClusterNodes.end(); ++iter)
    {
        if (iter->nodeId == currentNodeId)
        {
            currentNodeName = iter->nodeName;
        }
    }

    if (currentNodeName.empty())
    {
        err_msg << "Failed to initialize current node name as it is not present in node list. Current node id is: " << currentNodeId;
        m_ErroMessage = err_msg.str().c_str();
        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());
        status = SVE_FAIL;

        return status;
    }

    AddFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_NAME, currentNodeName);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return status;

}

SVSTATUS FailoverClusterInfo::GetClusterId(std::string& id)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVSTATUS status = SVS_OK;
    stringstream err_msg;

    if (getStringValue("Cluster", FailoverCluster::FAILOVER_CLUSTER_ID, id) != SVS_OK)
    {
        status = SVE_FAIL;

        err_msg << "Failed to get the cluster instance Id from registry";
        m_ErroMessage = err_msg.str();

        DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, m_ErroMessage.c_str());
        
        return status;
    }

    AddFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_ID, id);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    
    return status;
}

std::set<NodeEntity> FailoverClusterInfo::GetClusterNodeSet()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return m_ClusterNodes;
}

void FailoverClusterInfo::dumpInfo()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    DebugPrintf(SV_LOG_DEBUG, "\n---- FAILOVER CLUSTER INFO----    \n");
    DebugPrintf(SV_LOG_DEBUG, "%s: %s\n", FailoverCluster::FAILOVER_CLUSTER_ID, GetFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_ID).c_str());
    DebugPrintf(SV_LOG_DEBUG, "%s: %s\n", FailoverCluster::FAILOVER_CLUSTER_NAME, GetFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_NAME).c_str());
    DebugPrintf(SV_LOG_DEBUG, "%s: %s\n", FailoverCluster::FAILOVER_CLUSTER_TYPE, GetFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_TYPE).c_str());
    DebugPrintf(SV_LOG_DEBUG, "%s: %s\n", FailoverCluster::FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_ID, GetFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_ID).c_str());
    DebugPrintf(SV_LOG_DEBUG, "%s: %s\n", FailoverCluster::FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_NAME, GetFailoverClusterProperty(FailoverCluster::FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_NAME).c_str());

    DebugPrintf(SV_LOG_DEBUG, "cluster version is: %d, %d\n", m_ClusterVersionInfo.MajorVersion, m_ClusterVersionInfo.MinorVersion);
    
    DebugPrintf(SV_LOG_DEBUG, "\nNODES INFO:\n");

    std::set<NodeEntity>::iterator iterNode;
    for (iterNode = m_ClusterNodes.begin(); iterNode != m_ClusterNodes.end(); ++iterNode)
    {
        DebugPrintf(SV_LOG_DEBUG, "node name: %s, node id: %s, node state: %d\n", (*iterNode).nodeName.c_str(), (*iterNode).nodeId.c_str(), iterNode->nodeState);
    }

    DebugPrintf(SV_LOG_DEBUG, "\nRESOURCES INFO:\n");

    std::set<ResourceEntity>::iterator iterResource;
    for (iterResource = m_ClusterResources.begin(); iterResource != m_ClusterResources.end(); ++iterResource)
    {
        DebugPrintf(SV_LOG_DEBUG, "resource name: %s, resource id: %s, node type: %s\n", (*iterResource).resourceName.c_str(), (*iterResource).resourceId.c_str(), (*iterResource).resourceType.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}