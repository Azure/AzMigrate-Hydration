#include "svsemaphore.h"
#include <iostream>
#include <ace/OS.h>
#include <aclapi.h>


SVSemaphore::SVSemaphore(const std::string &name,
						 unsigned int count, // By default make this unlocked.
						 int max)
{
	m_name = name;
	m_semaphore = NULL;
	m_owner = 0;


	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
	PACL pACL = NULL;
	PSECURITY_DESCRIPTOR pSD = NULL;
	SECURITY_ATTRIBUTES sa;
	EXPLICIT_ACCESS ea[2];

	// Create a well-known SID for the Everyone group.
	if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
		SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0,
		&pEveryoneSID))
	{
		//        DebugPrintf(SV_LOG_ERROR,"AllocateAndInitializeSid failed. %s\n", 
		//            Error::Msg().c_str());
		std::cerr << "AllocateAndInitializeSid failed. errno:" << GetLastError() << std::endl;
		goto Cleanup;
	}

	// Create a SID for the BUILTIN\Administrators group.
	if(! AllocateAndInitializeSid(&SIDAuthNT, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&pAdminSID)) 
	{
		//        DebugPrintf(SV_LOG_ERROR,"AllocateAndInitializeSid failed. %s\n", 
		//            Error::Msg().c_str());
		std::cerr << "AllocateAndInitializeSid failed. errno:" << GetLastError() << std::endl;
		goto Cleanup; 
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow Everyone read access to the key.
	ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
	ea[0].grfAccessPermissions = SEMAPHORE_ALL_ACCESS;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance= NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;


	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow the Administrators group full access to
	// the key.
	ea[1].grfAccessPermissions = SEMAPHORE_ALL_ACCESS;
	ea[1].grfAccessMode = SET_ACCESS;
	ea[1].grfInheritance= NO_INHERITANCE;
	ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
	ea[1].Trustee.ptstrName  = (LPTSTR) pAdminSID;


	// Create a new ACL that contains the new ACEs.
	DWORD dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
	if (ERROR_SUCCESS != dwRes) 
	{
		//        DebugPrintf(SV_LOG_ERROR,"SetEntriesInAcl failed. %s\n", 
		//            Error::Msg().c_str());
		std::cerr << "SetEntriesInAcl failed. errno:" << GetLastError() << std::endl;
		goto Cleanup;
	}

	// Initialize a security descriptor.  
	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
		SECURITY_DESCRIPTOR_MIN_LENGTH); 
	if (NULL == pSD) 
	{ 
		//        DebugPrintf(SV_LOG_ERROR,"LocalAlloc failed. %s\n", 
		//            Error::Msg().c_str());
		std::cerr << "LocalAlloc failed. errno:" << GetLastError() << std::endl;
		goto Cleanup; 
	}


	if (!InitializeSecurityDescriptor(pSD,
		SECURITY_DESCRIPTOR_REVISION)) 
	{  
		//        DebugPrintf(SV_LOG_ERROR,"InitializeSecurityDescriptor failed. %s\n", 
		//            Error::Msg().c_str());
		std::cerr << "InitializeSecurityDescriptor failed. errno:" << GetLastError() << std::endl;
		goto Cleanup; 
	}


	// Add the ACL to the security descriptor. 
	if (!SetSecurityDescriptorDacl(pSD, 
		TRUE,     // bDaclPresent flag   
		pACL, 
		FALSE))   // not a default DACL 
	{  
		//        DebugPrintf(SV_LOG_ERROR,"SetSecurityDescriptorDacl failed. %s\n", 
		//            Error::Msg().c_str());
		std::cerr << "SetSecurityDescriptorDacl failed. errno:" << GetLastError() << std::endl;
		goto Cleanup; 
	}


	// Initialize a security attributes structure.
	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;



	m_semaphore = ::CreateSemaphore(&sa,count,max,name.c_str());
	if(!m_semaphore)
	{
		//        DebugPrintf(SV_LOG_ERROR,"semaphore (%s) creation  failed. %s\n", 
		//            m_name.c_str(), Error::Msg().c_str());
		std::cerr << "semaphore (" << m_name << ") creation  failed. errno:" << GetLastError() << std::endl;
	}


Cleanup:

	if (pEveryoneSID) 
		FreeSid(pEveryoneSID);
	if (pAdminSID) 
		FreeSid(pAdminSID);
	if (pACL) 
		LocalFree(pACL);
	if (pSD) 
		LocalFree(pSD);

}

SVSemaphore::~SVSemaphore()
{
    if(m_semaphore)
    {
        if(m_owner == ACE_OS::thr_self())
            release();
        ::CloseHandle(m_semaphore);
}
}

bool SVSemaphore::acquire()
{
    DWORD dwEvent = WAIT_TIMEOUT;

    if(!m_semaphore)
        return false;

    // If same thread calls acquire again
    // without releasing, we return success
	if(m_owner == ACE_OS::thr_self())
		return true;

	// P ( m_semaphore )
    dwEvent = ::WaitForSingleObject(m_semaphore, INFINITE);
    if(WAIT_OBJECT_0 == dwEvent)
    {
		m_owner = ACE_OS::thr_self();
        return true;
    }

    return false;
}

bool SVSemaphore::tryacquire()
{
    DWORD dwEvent = WAIT_TIMEOUT;

    if(!m_semaphore)
        return false;

    // If same thread calls acquire again
    // without releasing, we return success
	if(m_owner == ACE_OS::thr_self())
        return true;

    // P ( m_semaphore )
    dwEvent = ::WaitForSingleObject(m_semaphore, 0);
    if(WAIT_OBJECT_0 == dwEvent)
    {
        m_owner = ACE_OS::thr_self();
        return true;
    }

    return false;
}

bool SVSemaphore::release()
{
    if(!m_semaphore)
        return false;

	if(m_owner == ACE_OS::thr_self())
    {
		m_owner = 0;
        ::ReleaseSemaphore(m_semaphore,1,NULL);
		return true;
    }

    return false;
}
