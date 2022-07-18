include top.mak

SRCS = \
	svconfig.cpp \
	volumegroupsettings.cpp \
	pushInstallationSettings.cpp \
	apiwrapper.cpp \
	talwrapper.cpp \
	xmlwrapper.cpp \
	marshal.cpp \
	unmarshal.cpp \
	xmlmarshal.cpp \
	xmlunmarshal.cpp \
	configuratorrpc.cpp \
	cxproxy.cpp \
	configurators.cpp \
	serializecompressmode.cpp \
	serializevolumegroupsettings.cpp \
	serializetransport_settings.cpp \
	serializeretentionsettings.cpp \
	serializeretentioninformation.cpp \
	serializeinitialsettings.cpp \
	serializecdpsnapshotrequest.cpp \
	serializeswitchinitialsettings.cpp \
	serializeatconfigmanagersettings.cpp \
	serializepushInstallationSettings.cpp \
	serializeapplicationsettings.cpp \
	serializeprismsettings.cpp \
	serializetask.cpp \
	serializevolumerecoveryhelperssettings.cpp\
	serializesourcesystemconfigsettings.cpp \
	xmlizecompressmode.cpp \
	xmlizevolumegroupsettings.cpp \
	xmlizetransport_settings.cpp \
	xmlizeretentionsettings.cpp \
	xmlizeretentioninformation.cpp \
	xmlizeinitialsettings.cpp \
	xmlizecdpsnapshotrequest.cpp \
	xmlizeatconfigmanagersettings.cpp \
	xmlizepushInstallationSettings.cpp \
	xmlizeapplicationsettings.cpp \
	xmlizeprismsettings.cpp \
	xmlizetask.cpp \
	xmlizevolumerecoveryhelperssettings.cpp\
	xmlizesourcesystemconfigsettings.cpp \
	switchrpcconfigurator.cpp \
	configureswitchagentproxy.cpp \
	switchinitialsettings.cpp \
	rpcconfigurator.cpp \
	configurevxagentproxy.cpp \
	retentionsettings.cpp \
	retentioninformation.cpp \
	initialsettings.cpp \
	cdpsnapshotrequest.cpp \
	fileconfigurator.cpp \
	configwrapper.cpp \
	prismsettings.cpp \
	datacacher.cpp \
	sourcesystemconfigsettings.cpp \
	$(X_ARCH)/fileconfiguratorex.cpp \
	$(X_ARCH)/svsemaphore.cpp \
	$(X_MINOR_ARCH)/svsemaphoreminor.cpp \
	cdpdef.cpp \
	snapshotmanagerproxy.cpp \
	protecteddevicedetails.h \

X_CFLAGS = \
	-Icommon \
    -Iinmsafeint \
    -Iinmsafeint/$(X_MAJOR_ARCH) \
    -Iscopeguard \
    -Isecuritylib \
    -Isecuritylib/$(X_MAJOR_ARCH) \
    -Isecuritylib/$(X_MINOR_ARCH) \
	-Iconfig \
	-IInmXmlParser \
	-Inpwwn \
	-Iappframeworklib \
	-Iappframeworklib/config \
	-Iconfig/$(X_MAJOR_ARCH) \
	-Iconfig/$(X_MINOR_ARCH) \
	-Icommon/$(X_MAJOR_ARCH) \
	-Is2libs/thread \
	-Ilog \
	-Iinmsafecapis \
	-Iinmsafecapis/$(X_MAJOR_ARCH) \
	-I$(X_ARCH)/config \
	-Ierrorexception \
	-Icxpslib \
	-Icxpslib/$(X_MINOR_ARCH) \
	-Icxpslib/$(X_MAJOR_ARCH) \
	-Icurlwrapperlib \
	$(ACE_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(CURL_INCLUDE) \
	$(OPENSSL_INCLUDE) \
	$(ZLIB_INCLUDE) \
	$(INM_MD5_INCLUDE) \
	$(SIGSLOT_INCLUDE) \
	$(ESJSON_INCLUDES) \


#-DDEBUG_XMLIZE \
#-DUSE_INMAGE_EXCEPTION_CALLSTACK \

include config/Makefile.4

include bottom.mak
