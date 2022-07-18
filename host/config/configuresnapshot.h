
#ifndef CONFIGURESNAPSHOT__H
#define CONFIGURESNAPSHOT__H

struct ConfigureSnapshot {
    virtual int getId() = 0;

    // vx action
    virtual void setSnapshotProgress() = 0;
    virtual void clearSnapshot() = 0;
};

#endif // CONFIGURESNAPSHOT__H

