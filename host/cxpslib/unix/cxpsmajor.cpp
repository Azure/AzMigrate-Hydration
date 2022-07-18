#include "cxps.h"

CSMode GetCSMode()
{
	return CS_MODE_LEGACY_CS;
}

const PSInstallationInfo& GetRcmPSInstallationInfo()
{
	throw std::runtime_error("Rcm PS is unsupported on Unix");
}

void idempotentRestorePSState()
{
	// NO-OP
}
