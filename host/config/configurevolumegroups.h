

struct ConfigureVolumeGroups {
    boost::range<> getGroups();
    void addGroup( groupInfo );
    void removeGroup( groupInfo );
};

