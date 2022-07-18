include top.mak

SRCS = \
	ConfiguratorAPITestBed.cpp \
	Globals.cpp \
	PrintFunctions.cpp \
	TestGetInitialSettings.cpp \
	TestResyncProgress.cpp \
	TestVolumeManager.cpp \
	TestSnapshotAPIs.cpp \

# BUGBUG: DrvDefines.h defined only in win32 directory
X_CFLAGS = \
	-DDP_PORTABLE \
	-DBASIC_TEST \
	-Iconfig/ConfiguratorAPITestBed \
	-Icommon \
    -Iinmsafeint \
    -Iinmsafeint/$(X_MAJOR_ARCH) \
	-Icommon/unix \
	-Iconfig/unix \
	-I$(X_ARCH)/config \
	-Ilog \
	-Iconfig \
	-Ital \
	-ISnapShotMgr/include \
	-Icdplibs -Icdplibs/v1 -Icdplibs/v3 \
	-IRetention \
	-Idrivers/Include/win32 \
	-Is2libs/common \
	-Is2libs \
	-Is2libs/foundation \
	-Is2libs/thread \
	-Idataprotection \
	-IAzureRecoveryLib/resthelper \
	$(ACE_INCLUDE) \
	$(SQLITE_INCLUDE) \
	$(BOOST_INCLUDE) \

X_LIBS = \
	config \
	common \
	tal \
	log \
	cdplibs \
	s2libs \
	volumeinfocollector \
    deviceidinformer \
    platforminfo \
    executecommand \
    scsicommandissuer \
    dirbasename \
	AzureRecoveryLib \

X_THIRDPARTY_LIBS = \
	$(ACE_LIBS) \
	$(SQLITE_LIBS) \
	$(ARES_LIBS) \
	$(CURL_LIBS) \
	$(OPENSSL_LIBS) \
	$(BOOST_LIBS)

X_SYSTEM_LIBS = \
	-lz \
	-lrt \

X_LDFLAGS = 

BINARY=ConfiguratorAPITestBed
BINARY_EXT=$(X_EXEEXT)

include bottom.mak

