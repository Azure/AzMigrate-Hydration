
struct ConfigureVolume {
    // vx actions
    void setVolumeInfo( capacity, freeSpace );
    void setVisible( bool visible );
    void setReadOnly( bool readOnly );

    // cx settings
    bool isVisibleReadOnly();
    bool isVisibleReadWrite();
    
    // callbacks
    VolumeInfo getVolumeInfo();
};

