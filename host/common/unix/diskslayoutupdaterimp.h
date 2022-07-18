#ifndef DISKS_LAYOUT_UPDATER_IMP_H
#define DISKS_LAYOUT_UPDATER_IMP_H

#include "diskslayoutupdater.h"
#include "configurator2.h"

class DisksLayoutUpdaterImp : public DisksLayoutUpdater
{
public:
    DisksLayoutUpdaterImp(Configurator *configurator) { }
    ~DisksLayoutUpdaterImp() { }
    void Update(const std::string &diskslayoutoption, Options_t & statusmap) { }
    void Clear(const Options_t & statusmap) { }
};

#endif /* DISKS_LAYOUT_UPDATER_IMP_H */
