#include "pushInstallationSettings.h"
#include <cstring>
using namespace std;

/*
* Function Name: strictCompare() 
* Arguments: 
*                  PushInstallationSettings 
* Return Value:
*                  true --> if the current object and argument PushInstallationSettings are same..
*                  false --> if the above case fails
* Description:
*                  This function compares strictly the given structures....
* Called by: 
*                  
*                  
* Calls:
*                   
*                   
* Issues Fixed:
*   Issue:
*   Root Cause:
*   Fix:
*/

bool PushInstallationSettings::strictCompare(PushInstallationSettings & piSettings)
{
    if( ispushproxy != piSettings.ispushproxy )
    {
        return false;
    }
    if(m_maxPushInstallThreads != piSettings.m_maxPushInstallThreads)
    {
        return false;
    }
    if( piRequests.size() != piSettings.piRequests.size() )
    {
        return false;
    } 
    std::list<PushRequests>::iterator newPushRequestIter = piSettings.piRequests.begin();
    std::list<PushRequests>::iterator origPushRequestIter = piRequests.begin();

    while( origPushRequestIter != piRequests.end() )
    {
        if(  strcmp( newPushRequestIter->username.c_str(), origPushRequestIter->username.c_str() ) != 0
            || strcmp( newPushRequestIter->password.c_str(), origPushRequestIter->password.c_str() ) != 0
            || strcmp( newPushRequestIter->domain.c_str(), origPushRequestIter->domain.c_str() ) != 0
            || newPushRequestIter->prOptions.size() != origPushRequestIter->prOptions.size() )
        {
            return false;
        }
        std::list<PushRequestOptions>::iterator newPrOptionsIter = newPushRequestIter->prOptions.begin();
        std::list<PushRequestOptions>::iterator origPrOptionsIter = origPushRequestIter->prOptions.begin();
        
        while( origPrOptionsIter != origPushRequestIter->prOptions.end() )
        {
            if( strcmp( newPrOptionsIter->installation_path.c_str(), origPrOptionsIter->installation_path.c_str() ) != 0 
                || newPrOptionsIter->jobId.c_str(), origPrOptionsIter->jobId.c_str() != 0 
                || strcmp( newPrOptionsIter->ip.c_str(), origPrOptionsIter->ip.c_str() ) != 0
                || newPrOptionsIter->state != origPrOptionsIter->state
                || newPrOptionsIter->reboot_required != origPrOptionsIter->reboot_required
				|| newPrOptionsIter->vmType != origPrOptionsIter->vmType
				|| strcmp(newPrOptionsIter->vCenterIP.c_str(), origPrOptionsIter->vCenterIP.c_str()) != 0
				|| strcmp(newPrOptionsIter->vCenterUserName.c_str(), origPrOptionsIter->vCenterUserName.c_str()) != 0
				|| strcmp(newPrOptionsIter->vCenterPassword.c_str(), origPrOptionsIter->vCenterPassword.c_str()) != 0
				|| strcmp(newPrOptionsIter->vmName.c_str(), origPrOptionsIter->vmName.c_str()) != 0
				|| strcmp(newPrOptionsIter->vmAccountId.c_str(), origPrOptionsIter->vmAccountId.c_str()) != 0
				|| strcmp(newPrOptionsIter->vcenterAccountId.c_str(), origPrOptionsIter->vcenterAccountId.c_str()) != 0
				|| strcmp(newPrOptionsIter->ActivityId.c_str(), origPrOptionsIter->ActivityId.c_str()) != 0
				|| strcmp(newPrOptionsIter->ClientRequestId.c_str(), origPrOptionsIter->ClientRequestId.c_str()) != 0
				|| strcmp(newPrOptionsIter->ServiceActivityId.c_str(), origPrOptionsIter->ServiceActivityId.c_str()) != 0)
            {
                return false ;
            }
            newPrOptionsIter++;
            origPrOptionsIter++;
        }
        
        newPushRequestIter++;
        origPushRequestIter++;
    } 
    return true;
    
}

/*
*	Copy constructors.
*/

PushInstallationSettings::PushInstallationSettings(const PushInstallationSettings & rhs)
{
	ispushproxy = rhs.ispushproxy;
	m_maxPushInstallThreads = rhs.m_maxPushInstallThreads;
	piRequests.assign(rhs.piRequests.begin(), rhs.piRequests.end());
}

PushRequests::PushRequests(const PushRequests & rhs)
{
	os_type = rhs.os_type;
	username = rhs.username;
	password = rhs.password;
	domain = rhs.domain;
	prOptions.assign(rhs.prOptions.begin(), rhs.prOptions.end());
}

/*
* Function Name:    overloading equal to( = ) operator 
* Arguements: 
*                  PushInstallationSettings 
* Return Value:
*                  the newly constructed structure
* Description:
*                  This fiunction copies the given pushinstallation structure into owner structure../
* Called by: 
*                  
*                  assign() function of the list...
* Calls:
*                   
*                   
* Issues Fixed:
*   Issue:
*   Root Cause:
*   Fix:
*/

PushInstallationSettings& PushInstallationSettings::operator=( const PushInstallationSettings & piSettings)
{
	if (this == &piSettings) return *this;

    ispushproxy = piSettings.ispushproxy;
    m_maxPushInstallThreads = piSettings.m_maxPushInstallThreads;
    piRequests.assign(piSettings.piRequests.begin(), piSettings.piRequests.end());
    
    return *this;
}

PushRequests& PushRequests::operator=(const PushRequests & rhs)
{
	if (this == &rhs) return *this;

	os_type = rhs.os_type;
	username = rhs.username;
	password = rhs.password;
	domain = rhs.domain;
	prOptions.assign(rhs.prOptions.begin(), rhs.prOptions.end());

	return *this;
}

/*
* Function Name:    overloading comapation double equal to( == ) operator 
* Arguements: 
*                  PushInstallationSettings 
* Return Value:     
*                  true --> if two owner and argument PushInstallationSettings are same..
*                  false --> if the above case fails 
*                  
* Description:
*                  This fiunction compares the given structures....
* Called by: 
*                  
*                  
* Calls:
*                   
*                   
* Issues Fixed:
*   Issue:
*   Root Cause:
*   Fix:
*/

bool PushInstallationSettings::operator==( PushInstallationSettings & piSettings) 
{
    if( ispushproxy != piSettings.ispushproxy )
    {
        return false;
    }
    if(m_maxPushInstallThreads != piSettings.m_maxPushInstallThreads)
    {
        return false;
    }
    if( piRequests.size() != piSettings.piRequests.size() )
    {
        return false;
    } 
    std::list<PushRequests>::iterator newPushRequestIter = piSettings.piRequests.begin();
    std::list<PushRequests>::iterator origPushRequestIter = piRequests.begin();
    
    while( origPushRequestIter != piRequests.end() )
    {
        std::string origUser, newUser, origPasswd, newPasswd, origDomain, newDomain ;
        origUser = origPushRequestIter->username ;
        newUser = newPushRequestIter->username ;
        origPasswd = origPushRequestIter->password ;
        newPasswd = newPushRequestIter->password ;
        origDomain = origPushRequestIter->domain ;
        newDomain = newPushRequestIter->domain ;
        int origSize, newSize ;
        origSize = origPushRequestIter->prOptions.size() ;
        newSize = newPushRequestIter->prOptions.size() ;
        if(origUser.compare(newUser) != 0 || 
            origPasswd.compare(newPasswd) != 0 || 
            origDomain.compare(newDomain) != 0 ||
            origSize != newSize )
        {
            return false;
        }
        std::list<PushRequestOptions>::iterator newPrOptionsIter = newPushRequestIter->prOptions.begin();
        std::list<PushRequestOptions>::iterator origPrOptionsIter = origPushRequestIter->prOptions.begin();
        
        while( origPrOptionsIter != origPushRequestIter->prOptions.end() )
        {
            std::string origIp, newIp ;
            origIp = origPrOptionsIter->ip ;
            newIp = newPrOptionsIter->ip ;
            if( strcmp( newPrOptionsIter->installation_path.c_str(), origPrOptionsIter->installation_path.c_str() ) != 0 
                || strcmp( newPrOptionsIter->ip.c_str(), origPrOptionsIter->ip.c_str() ) != 0
                || newPrOptionsIter->reboot_required != origPrOptionsIter->reboot_required 
                || newPrOptionsIter->jobId.compare(origPrOptionsIter->jobId) != 0 
				|| newPrOptionsIter->vCenterIP.compare(origPrOptionsIter->vCenterIP) != 0
				|| newPrOptionsIter->vCenterUserName.compare(origPrOptionsIter->vCenterUserName) != 0
				|| newPrOptionsIter->vCenterPassword.compare(origPrOptionsIter->vCenterPassword) != 0
				|| newPrOptionsIter->vmName.compare(origPrOptionsIter->vmName) != 0
				|| newPrOptionsIter->vmType.compare(origPrOptionsIter->vmType) != 0
				|| newPrOptionsIter->vmAccountId.compare(origPrOptionsIter->vmAccountId) != 0
				|| newPrOptionsIter->vcenterAccountId.compare(origPrOptionsIter->vcenterAccountId) != 0
				|| newPrOptionsIter->ActivityId.compare(origPrOptionsIter->ActivityId) != 0
				|| newPrOptionsIter->ClientRequestId.compare(origPrOptionsIter->ClientRequestId) != 0
				|| newPrOptionsIter->ServiceActivityId.compare(origPrOptionsIter->ServiceActivityId) != 0)
            {
                return false ;
            }
            newPrOptionsIter++;
            origPrOptionsIter++;
        }
        
        newPushRequestIter++;
        origPushRequestIter++;
    } 
    
    return true;
} 
