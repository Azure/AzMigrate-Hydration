
include top.mak

SRCS = \
    fingerprintmgr.cpp \
    CertHelpers.cpp \
	getcertproperties.cpp \
    $(X_MINOR_ARCH)/setpermissionsminor.cpp \
    $(X_MAJOR_ARCH)/setpermissionsmajor.cpp \
    $(X_MAJOR_ARCH)/cryptomajor.cpp \
	$(X_MINOR_ARCH)/searchcertstoreminor.cpp

X_CFLAGS = \
   -Isecuritylib \
   -Isecuritylib/$(X_MINOR_ARCH) \
   -Isecuritylib/$(X_MAJOR_ARCH) \
   -Ierrorexception \
   -Ilog \
   -Icommon \
   -Icommon/$(X_MAJOR_ARCH) \
   -Iconfig \
   -Iconfig/$(X_MAJOR_ARCH) \
   $(BOOST_INCLUDE) \
   $(OPENSSL_INCLUDE) \
   $(CURL_INCLUDE) \
   -Iscopeguard \
    -Icxpslib \
    -Icxpslib/$(X_MINOR_ARCH) \
    -Icxpslib/$(X_MAJOR_ARCH) \
   -Icommon/$(X_MAJOR_ARCH) \
   -Iexecutecommand

X_LIBS = 

X_THIRDPARTY_LIBS = \
    $(BOOST_LIBS) \
    $(OPENSSL_LIBS) \
    $(CURL_LIBS)

X_SYSTEM_LIBS =

X_LDFLAGS =

BINARY=$(X_LIBPRE)securitylib
BINARY_EXT=$(X_LIBEXT)

include bottom.mak
