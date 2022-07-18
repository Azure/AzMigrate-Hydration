#ifndef TESTVOLUMEMANAGER_H
#define TESTVOLUMEMANAGER_H

#include <string>
#include <map>
#include "sigslot.h"

class TestVolumeManager:public sigslot::has_slots<>
{
public:
    TestVolumeManager();
    void onSetVolumeAttributes( std::map<std::string,int> const& deviceName );
    //void  onSetReadOnly(std::string const& device);
    //void  onSetReadWrite(std::string const& device);
    //void  onSetVisible(std::string const& device);
    //void  onSetInvisible(std::string const& device);
    void TestGetVolumeCheckpoint();
};


#endif

