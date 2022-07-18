
#ifndef CONFIGUREVOLUMEMANAGER__H
#define CONFIGUREVOLUMEMANAGER__H

#include <string>
#include "sigslot.h"

struct ConfigureVolumeManager {
    // vx actions
    //void addVolume( volumeInfo );
    //void removeVolume( volume );
    //void renameVolume( oldVolume, newVolumeName );

    // callbacks
    sigslot::signal1<std::string const&> setReadOnly;
    sigslot::signal1<std::string const&> setReadWrite;
    sigslot::signal1<std::string const&> setVisible;
    sigslot::signal1<std::string const&> setInvisible;
};

#endif // CONFIGUREVOLUMEMANAGER__H

