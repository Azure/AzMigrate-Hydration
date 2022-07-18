

struct ConfigureReplicationPairManager {
    boost::range<> getPairs();

    void startReplication( pair ); // maybe add these to pair instead
    void stopReplication( pair );
};

