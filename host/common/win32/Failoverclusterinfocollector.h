/*
    FailoverInfoCollector.h: Failover cluster related information collector
*/

#ifndef FAILOVER_CLUSTER_INFO_COLLECTOR_H
#define FAILOVER_CLUSTER_INFO_COLLECTOR_H

#include <string>
#include <map>
#include "svtypes.h"
#include <boost/thread/mutex.hpp>
#include "volumegroupsettings.h"

#include <ClusApi.h>
#pragma comment( lib, "ClusAPI.lib" )

#define FAILOVER_CLUSTER_GENERIC_BUFFER_LEN 1024

namespace FailoverCluster {

    const char FAILOVER_CLUSTER_ID[] = "clusterinstanceid";
    const char FAILOVER_CLUSTER_NAME[] = "clustername";
    const char FAILOVER_CLUSTER_MANAGEMENT_ID[] = "clustermanagementid";
    const char FAILOVER_CLUSTER_TYPE[] = "clustertype";
    const char FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_NAME[] = "clustercurrentnodename";
    const char FAILOVER_CLUSTER_CURRENT_CLUSTER_NODE_ID[] = "clustercurrentnodeid";

    const char DEFAULT_FAILOVER_CLUSTER_TYPE[] = "WindowsServerFailoverCluster";

    typedef enum {
        ClusterStateNotInstalled,
        ClusterStateNotConfigured,
        ClusterStateNotRunning,
        ClusterStateRunning,
        ClusterStateUnknown
    } ClusterServiceStatus;

    typedef enum {
        ClusterNodeStateUnknown = -1,
        ClusterNodeUp,
        ClusterNodeDown,
        ClusterNodePaused,
        ClusterNodeJoining
    } CLUSTER_NODE_STATE;

}

struct NodeEntity {

    std::string nodeName;

    /// \brief represents the node id in the cluster, node id remains same even if the node name is changed
    std::string nodeId;

    /// \brief about the node state in the cluster
    FailoverCluster::CLUSTER_NODE_STATE nodeState = FailoverCluster::CLUSTER_NODE_STATE::ClusterNodeStateUnknown;

    NodeEntity(std::string name, std::string id, FailoverCluster::CLUSTER_NODE_STATE state) : nodeName(name), nodeId(id), nodeState(state) {}
    NodeEntity() {}

    bool operator < (const NodeEntity& rhs) const
    {
        return nodeId < rhs.nodeId;
    }

};

struct ResourceEntity {
    
    /// \brief represents the name of the resource
    std::string resourceId;

    /// \brief represents the name of the resource
    std::string resourceName;

    /// \brief represents the type of the resource
    std::string resourceType;

    ResourceEntity() {}

    ResourceEntity(std::string id, std::string name, std::string type) : resourceId(id), resourceName(name), resourceType(type) {}

    bool operator < (const ResourceEntity& rhs) const
    {
        return resourceId < rhs.resourceId;
    }
};

class FailoverClusterInfo;
typedef FailoverClusterInfo* FailoverClusterInfoPtr;

class FailoverClusterInfo
{

private:


    /// \brief about the cluster version
    CLUSTERVERSIONINFO m_ClusterVersionInfo;

    /// \brief represents the last error message encountered during the cluster operations
    std::string m_ErroMessage;

    /// \brief represents the properties of a failover cluster
    std::map<std::string, std::string> m_Properties;

    /// \brief represents the list of nodes in the failover cluster
    std::set<NodeEntity> m_ClusterNodes;

    /// \brief represents the list of the resources in the failover cluster
    std::set<ResourceEntity> m_ClusterResources;

    VolumeSummaries_t m_volumeSummaries;

    VolumeDynamicInfos_t m_volumeDynamicInfos;

    SVSTATUS GetFailoverClusterName();

    SVSTATUS GetFailoverClusterNodesInfo();

    SVSTATUS GetCurrentClusterNodesInfo();

    SVSTATUS GetClusterId(std::string& id);

    SVSTATUS GetClusterResources();
    
    SVSTATUS GetReosurceInfo(HCLUSTER hcluster, std::string& resourceName, ResourceEntity& entity);


public:

    FailoverClusterInfo() {}

    ~FailoverClusterInfo() {}

    bool IsClusterNode();

    bool GetClusSvcStatusOnCurrentNode(FailoverCluster::ClusterServiceStatus& status);

    std::string GetLastErrorMessage(void);

    void AddFailoverClusterProperty(const std::string& key, const std::string& value);

    std::string GetFailoverClusterProperty(const std::string& key);

    SVSTATUS CollectFailoverClusterProperties(bool collectClusterDiskInformationAlso);

    SVSTATUS CollectFailoverClusterProperties();

    std::set<NodeEntity> GetClusterNodeSet();

    void dumpInfo();

};

#endif