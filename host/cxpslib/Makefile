include top.mak

SRCS = \
	cfscontrolclient.cpp \
	cfsmanager.cpp \
	cxpslogger.cpp \
	cxpsmsgqueue.cpp \
	requesthandler.cpp \
	server.cpp \
	serverctl.cpp \
	serveroptions.cpp \
	session.cpp \
	ThrottlingHelper.cpp \
	DiffResyncThrottlingHelper.cpp \
	pssettingsconfigurator.cpp \
	$(X_MAJOR_ARCH)/cfsserver.cpp \
	$(X_MAJOR_ARCH)/cfssession.cpp \
	$(X_MAJOR_ARCH)/requesthandlermajor.cpp \
	$(X_MINOR_ARCH)/sessionidminor.cpp \
	Telemetry/cxpstelemetrylogger.cpp \
	Telemetry/SourceFilePathParser.cpp \
	Telemetry/TelemetryRows.cpp \
	$(X_MAJOR_ARCH)/cxpsmajor.cpp

X_CFLAGS = \
	-Icxpslib \
	-Icxpslib/$(X_MINOR_ARCH) \
	-Icxpslib/$(X_MAJOR_ARCH) \
	-Isecuritylib \
	-Isecuritylib/$(X_MAJOR_ARCH) \
	-Icommon \
	-Iinmsafeint \
	-Iinmsafeint/$(X_MAJOR_ARCH) \
	-Iinmsafecapis \
	-Iinmsafecapis/$(X_MAJOR_ARCH) \
	-Icommon/$(X_MAJOR_ARCH) \
	-Iconfig \
	-Ierrorexception \
	-Ierrorexception/$(X_MINOR_ARCH) \
	-Ifio \
	-Ifio/$(X_MINOR_ARCH) \
	-Ifio/$(X_MAJOR_ARCH) \
	-Ilistfile \
	-Ilistfile/$(X_MAJOR_ARCH) \
	-Iscopeguard \
	-Isimplelogger \
	-Isimplelogger/$(X_MAJOR_ARCH) \
	-Izflate \
	-IAzureRecoveryLib/resthelper \
	$(OPENSSL_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(ZLIB_INCLUDE) \
	$(ESJSON_INCLUDES) \
	$(INM_MD5_INCLUDE) \
	$(CURL_INCLUDE) \
	$(ACE_INCLUDE) \
	-DESJ_NO_PRETTY_PRINT

X_LIBS = 

X_THIRDPARTY_LIBS =

X_SYSTEM_LIBS =

X_LDFLAGS =

BINARY=$(X_LIBPRE)cxpslib
BINARY_EXT=$(X_LIBEXT)

include bottom.mak


