include top.mak
X_OS:=$(shell ../build/scripts/general/OS_details.sh 1)

SRCS =  \
	main.cpp 

X_CFLAGS = \
	-IAzureRecoveryLib \
	$(BOOST_INCLUDE) \
	$(ACE_INCLUDE) \
	$(OPENSSL_INCLUDE) \
	$(ZLIB_INCLUDE) \
	$(CURL_INCLUDE) \
	$(LIBXML2_INCLUDE) 

X_LIBS = \
    AzureRecoveryLib \
    securitylib

X_THIRDPARTY_LIBS = \
	$(ACE_LIBS) \
	$(ARES_LIBS) \
	$(CURL_LIBS) \
	$(OPENSSL_LIBS) \
	$(ZLIB_LIBS) \
	$(BOOST_LIBS) \
	$(LIBXML2_LIBS)

ifeq ($(X_OS), RHEL7-64)
	X_SYSTEM_LIBS = -llzma
else
ifeq ($(X_OS), SLES11-SP4-64)
        X_SYSTEM_LIBS = -llzma
else
	X_SYSTEM_LIBS =
endif
endif

X_LDFLAGS =

BINARY=AzureRecoveryUtil
BINARY_EXT=$(X_EXEEXT)

include bottom.mak
