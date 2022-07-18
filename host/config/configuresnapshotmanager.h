//
// configuresnapshotmanager.h: interface to snapshot manager
//
#ifndef CONFIGURESNAPSHOTMANAGER__H
#define CONFIGURESNAPSHOTMANAGER__H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include "sigslot.h"
#include "configuresnapshot.h"

struct ConfigureSnapshotManager {
    // boost::range<> getSnapshots();
    typedef std::map<std::string, boost::shared_ptr<ConfigureSnapshot> > Range;
    virtual Range getSnapshots() = 0;

    // void removeSnapshot( snapshot );
    virtual int getSnapshotCount() = 0; // todo: remove
    sigslot::signal5<int, std::string const&, std::string const&, std::string const&, std::string const&> startSnapshot;
    sigslot::signal1<int> stopSnapshot;

    virtual ~ConfigureSnapshotManager() {}
};

#endif // CONFIGURESNAPSHOTMANAGER__H


