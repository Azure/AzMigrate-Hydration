#ifndef DISKS_LAYOUT_UPDATER_H
#define DISKS_LAYOUT_UPDATER_H

#include <string>
#include "portablehelpers.h"

class DisksLayoutUpdater
{
public:
	/* updates the disks layout and status is kept in status map */
    virtual void Update(const std::string &diskslayoutoption, Options_t & statusmap) = 0;
	/* based on status in status map, clears the disks layout */
    virtual void Clear(const Options_t & statusmap) = 0;

	virtual ~DisksLayoutUpdater() { }
};

#endif /* DISKS_LAYOUT_UPDATER_H */