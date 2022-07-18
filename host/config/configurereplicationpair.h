
struct ConfigureReplicationPair {
    void setResyncProgress(/*offset, bytes, matched, oldName, newName,delete*/);
    void setResyncRequired();

    boost::range<T> getSourceVolumes();
    boost::range<T> getTargetVolumes();
    bool shouldThrottleBootstrap();
    bool shouldThrottleResync();
    bool shouldThrottleDifferentials();
    bool shouldThrottleTargetAgent();
    bool shouldResync();
    bool isOffloadResync();
    bool isFastResync();
    ReplicationPairSecuritySettings getSecureSettings();

    void throttleBootstrap( bool throttle );
    void throttleResync( bool throttle );
    void throttleDifferentials( bool throttle );
    void throttleTargetAgent( bool throttle );
};

