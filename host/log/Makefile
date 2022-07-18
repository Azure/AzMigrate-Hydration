include top.mak

SRCS =  logger.cpp \

X_CFLAGS = \
	-Ilog\
	-Icommon \
    -Iinmsafeint \
    -Iinmsafeint/$(X_MAJOR_ARCH) \
	-Icommon/$(X_MAJOR_ARCH) \
	-Iconfig \
	-Iconfig/$(X_MAJOR_ARCH) \
	-Is2libs/thread \
	-Is2libs/common \
	-Iinmsafecapis \
	-Iinmsafecapis/$(X_MAJOR_ARCH) \
   -Isecuritylib \
   -Isecuritylib/$(X_MAJOR_ARCH) \
   -Ierrorexception \
	-Icxpslib \
	-Icxpslib/$(X_MINOR_ARCH) \
	-Icxpslib/$(X_MAJOR_ARCH) \
   -Icurlwrapperlib \
   $(OPENSSL_LIBS) \
	$(ACE_INCLUDE) \
	$(CURL_INCLUDE) \
	$(BOOST_INCLUDE) \
	$(SIGSLOT_INCLUDE) \
	$(OPENSSL_INCLUDE) \
	$(ESJSON_INCLUDES) \

#  -DUSE_INMAGE_EXCEPTION_CALLSTACK

X_LIBS = 

X_THIRDPARTY_LIBS = 

X_SYSTEM_LIBS =

X_LDFLAGS =

BINARY=$(X_LIBPRE)log
BINARY_EXT=$(X_LIBEXT)

include bottom.mak

