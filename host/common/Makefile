include top.mak

SRCS = \
	$(X_MAJOR_ARCH)/agenthelpersmajor.cpp \
	svmacros.cpp \
	$(X_MAJOR_ARCH)/portablemajor.cpp \
	$(X_MAJOR_ARCH)/service.cpp \
	portablehelpers.cpp \
	HealthCollator.cpp \
	appcommand.cpp \
	HttpFileTransfer.cpp \
	svutilities.cpp \
	svtypesstub.cpp \
	svutils.cpp \
	svtypes.cpp \
	hostrecoverymanager.cpp \
	$(X_MAJOR_ARCH)/dbgbreak.cpp \
	$(X_MAJOR_ARCH)/portablehelpersmajor.cpp \
	$(X_MAJOR_ARCH)/hostrecoverymanagermajor.cpp \
	$(X_MINOR_ARCH)/portablehelpersminor.cpp \
	proxysettings.cpp \
	ErrorLogger.cpp \
	ServiceHelper.cpp \
	inmcertutil.cpp

X_CFLAGS = \
    -IAzureRcmLib \
	-Icommon \
    -Iinmsafeint \
    -Iinmsafeint/$(X_MAJOR_ARCH) \
	-Inpwwn \
	-Icommon/$(X_MAJOR_ARCH) \
	-Icommon/$(X_MINOR_ARCH) \
	-Is2libs/common \
	-Iconfig \
	-Iconfig/$(X_MAJOR_ARCH) \
	-I$(X_ARCH)/config \
	-Ilog \
	-Ital \
	-Icxtransportlib \
	-Izflate \
	-Ierrorexception \
	-Iscopeguard \
    -Ifio \
    -Ifio/$(X_MINOR_ARCH) \
    -Ifio/$(X_MAJOR_ARCH) \
	-Isimplelogger \
	-Isimplelogger/$(X_MAJOR_ARCH) \
	-Icxpslib \
	-Icxpslib/$(X_MINOR_ARCH) \
	-Icxpslib/$(X_MAJOR_ARCH) \
	-Ilistfile \
    -Ilistfile/$(X_MAJOR_ARCH) \
	-Is2libs/thread \
	-Is2libs/foundation \
	-Is2libs/streams \
	-Iservice \
	-Idevicefilter \
    -Idevicefilter/$(X_MAJOR_ARCH) \
    -Idrivers/InVolFlt/common \
	-Idrivers/InVolFlt/$(X_MINOR_ARCH) \
	-Idrivers/lib/$(X_MINOR_ARCH) \
    -Ideviceidinformer \
    -Iplatforminfo \
    -Iplatforminfo/$(X_MAJOR_ARCH) \
    -Iplatforminfo/$(X_MINOR_ARCH) \
    -Iexecutecommand \
    -Iexecutecommand/$(X_MINOR_ARCH) \
    -Iscsicommandissuer \
    -Iscsicommandissuer/$(X_MINOR_ARCH) \
    -Idirbasename \
    -Idirbasename/$(X_MAJOR_ARCH) \
	-Ivolumeinfocollector \
	-Ivolumeinfocollector/$(X_MAJOR_ARCH) \
	-Ivolumeinfocollector/$(X_MINOR_ARCH) \
	-Icdplibs -Icdplibs/v1 -Icdplibs/v3 \
	-Idisklayoutmgr \
    -Iinmsafecapis \
    -Iinmsafecapis/$(X_MAJOR_ARCH) \
    -Ierrorexception \
    -Icurlwrapperlib \
	-Isecuritylib \
	-Isecuritylib/$(X_MAJOR_ARCH) \
    -Iinmuuid/$(X_MAJOR_ARCH) \
	$(SQLITE_INCLUDE) \
	$(ACE_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(CURL_INCLUDE) \
	$(OPENSSL_INCLUDE) \
	$(ZLIB_INCLUDE) \
	$(INM_MD5_INCLUDE) \
	$(SIGSLOT_INCLUDE) \
    $(ESJSON_INCLUDES)

X_LIBS = 

X_THIRDPARTY_LIBS = 

X_SYSTEM_LIBS =

X_LDFLAGS =

BINARY=$(X_LIBPRE)common
BINARY_EXT=$(X_LIBEXT)

include bottom.mak

