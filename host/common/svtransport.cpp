#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>        // close(), write()
#include "svtypes.h"
#include "svtransport.h"
#include "hostagenthelpers_ported.h"
#include "portable.h"
#include "../log/logger.h"
#define MAX_PORT_DIGITS 5

