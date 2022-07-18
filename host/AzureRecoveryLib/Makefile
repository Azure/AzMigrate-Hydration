include top.mak
SRCS = \
    resthelper/HttpUtil.cpp \
	resthelper/HttpClient.cpp \
	resthelper/HttpRequest.cpp \
	resthelper/HttpResponse.cpp \
	resthelper/CloudBlob.cpp \
	resthelper/ServiceBusQueue.cpp \
	resthelper/EventHub.cpp \
	common/Trace.cpp \
	common/utils.cpp \
	common/Process.cpp \
	config/LibXmlUtil.cpp \
	config/HostInfoConfig.cpp \
	config/RecoveryConfig.cpp \
	linux/BlockDeviceDetails.cpp \
	linux/SourceFilesystemTree.cpp \
	linux/LinuxUtils.cpp \
	linux/RecoveryHelpers.cpp \
	AzureRecovery.cpp 

X_CFLAGS = \
	-Icommon \
	-Iinmsafecapis \
	-Iinmsafecapis/$(X_MAJOR_ARCH) \
	-Isecuritylib \
	-Isecuritylib/$(X_MAJOR_ARCH) \
	-Isecuritylib/$(X_MINOR_ARCH) \
	$(ACE_INCLUDE) \
	$(CURL_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(ZLIB_INCLUDE) \
	$(OPENSSL_INCLUDE) \
	$(LIBXML2_INCLUDE)

X_LIBS = 

X_THIRDPARTY_LIBS = 

X_SYSTEM_LIBS =

X_LDFLAGS =

BINARY=$(X_LIBPRE)AzureRecoveryLib
BINARY_EXT=$(X_LIBEXT)

include bottom.mak


