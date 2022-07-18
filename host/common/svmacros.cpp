#include <string>
using namespace std;
#ifdef HAVE_UUID_GENERATE
#include <uuid/uuid.h>
#endif
#include "svtypes.h"
#include "svutils.h"
#include "svmacros.h"

SVERROR GET_NEW_UUID(string& guuid)
{
    #ifdef HAVE_UUID_GENERATE
    char guid[256];
    memset(guid,0,256);
    uuid_t unident;
    uuid_generate(unident);
    uuid_unparse(unident, guid);
    guuid = guid;
    #else
    GetProcessOutput("uuidgen", guuid);
    #endif
    return SVS_OK;
}
