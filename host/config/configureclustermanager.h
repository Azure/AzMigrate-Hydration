

struct ConfigureClusterManager {
    void addCluster( clusterInfo );
    void removeCluster( cluster ); 
    void addClusterVolume( cluster, volume );
    void removeClusterVolume( cluster, volume );
    void clusterFailedOver( clusterInfo );
};

