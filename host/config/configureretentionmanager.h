
struct ConfigureRetentionManager {
    // cx settings
    limits getRetentionLimits();
    time getRecoverTime();
    option getRetentionOption();
    MetaInfo getMeta();
    string getRetentionPreScript();
    string getRetentionPostScript();

    // callbacks
    void setRetentionLimits();
    void setRecoveryTime();
    void setRetentionOption();
    void setMeta();
    void setRetentionPreScript();
    void setRetentionPostScript();

};

