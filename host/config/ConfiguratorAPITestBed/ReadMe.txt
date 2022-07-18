========================================================================
    CONSOLE APPLICATION : ConfiguratorAPITestBed Project Overview
========================================================================

AppWizard has created this ConfiguratorAPITestBed application for you.  
This file contains a summary of what you will find in each of the files that
make up your ConfiguratorAPITestBed application.


ConfiguratorAPITestBed.vcproj
    This is the main project file for VC++ projects generated using an Application Wizard. 
    It contains information about the version of Visual C++ that generated the file, and 
    information about the platforms, configurations, and project features selected with the
    Application Wizard.

ConfiguratorAPITestBed.cpp
    This is the main application source file.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named ConfiguratorAPITestBed.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////
ConfiguratorAPITest.zip file contains following files
-----------------------------------------------------
(1) ConfiguratorAPITest.exe
(2) drscout.conf

Extract executable (1) to any location on your local machine.

Copy drscout.conf to <INSTALL ROOT>/conf/drscout.conf location on your target machine (example: C:\Program Files\InMage Systems\conf) and modify following keys.

 

(1)     Modify ‘Hostname’ to point to your CX IP.

(2)     Modify ‘HostId’ to your target HostID.

 

Rest of the settings assumes default install location.

Configure some replication pairs from GUI.

Run the executable with following arguments:

(1) ConfiguratorAPITest.exe -obj1 <objname> -mtd1 <methodname> -tdrive <target drive>

   For example to execute getInitialSettings, run as below:

  >> ConfiguratorAPITest.exe  -obj getVxAgent -mtd1 getInitialSettings -tdrive K

(2) Running ConfiguratorAPITest.exe without commandline arguments prompts as below:

  >>  No arguments to parse. Do you want to continue with standard test?:[Y|y]

    Type 'Y' to proceed with standard tests as programmed in application
    Type 'N" to quit

  Default drive for resync is "K".


Sample Output for getInitialSettings:

FTP_CONNECTION_SETTINGS:
ipAddress: 10.0.1.98    password: svsHillview   port: 21

-------------------------------------------------------------------
CDP_SETTINGS:

I  -----
CDP_STATE: 0
CDP_LOGTYPE: 0
m_metadir:
m_dbname:
m_altmetadir:
CDP_COMPRESSION_METHOD: 0
CDP_DELETE_OPTION: 0

-------------------------------------------------------------------
HOST_VOLUME_GROUP_SETTINGS:

Direction: TARGET       Id: 7   status: 0

VOLUME_SETTINGS:

VOLUMES:

I
deviceName: I
hostname: localhost
hostId: F99DA16D-85DB-3E4E-B80C70E3D80397F6
secureMode: 0
sourceToCXSecureMode: -858993460
synchmode: SYNC_OFFLOAD
transportProtocol: 0
visibility: 0
sourcecapacity: 0
resyncDirectory: /home/svsystems/F99DA16D-85DB-3E4E-B80C70E3D80397F6/G/re

-------------------------------------------------------------------
initialsettings.resyncRequired=1
initialsettings.shouldThrottle=0





