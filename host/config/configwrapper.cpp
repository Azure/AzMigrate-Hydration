//
// Copyright (c) 2008 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File       : configwrappers.h
//
// Description: 
//  exception free wrapper implementation over configurator calls
//

#include "configwrapper.h"
#include "configurator2.h"
#include "configurevxagent.h"
#include "inmageex.h"
#include "portablehelpersmajor.h"
#include "localconfigurator.h"
#include "rpcconfigurator.h"
#include "marshal.h"
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

boost::mutex g_AlertMutex;
static bool ignoreCxErrors = false;
static SV_LOG_LEVEL logLevel = SV_LOG_ERROR;

static ACE_Mutex g_lock;
static Configurator* g_ConfiguratorInstance = NULL;
bool Configurator::instanceFlag = false;


Configurator::Configurator()
{ 
    instanceFlag = true; 
}

Configurator::~Configurator()
{
    instanceFlag = false;
    g_ConfiguratorInstance = NULL;
}


/*
* FUNCTION NAME :  InitializeConfigurator
*
* DESCRIPTION : 
*   Create Configurator Instance
*
* INPUT PARAMETERS : 
*   configSource = Source for configuration
*
* OUTPUT PARAMETERS : 
*   pointer to configurator instance
*
* NOTES :
* 
*
* return value : true on success, otherwise false
*
*/

bool InitializeConfigurator(Configurator ** ppconfigurator, 
                            ConfigSource configSource, 
                            const SerializeType_t serializetype,
                            const std::string& cachepath)
{
    bool rv = false;
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    ACE_Guard<ACE_Mutex> guard( g_lock );

    do 
    {
        LocalConfigurator localConfigurator;

        try 
        {
            if(!ppconfigurator)
            {
                rv = false;
                break;
            }

            if(Configurator::instanceFlag)
            {
                // configurator was previously created
                // return the existing configurator
                // Configurator initialization (creation) shall
                // be done only once

                *ppconfigurator = g_ConfiguratorInstance;
                rv = true;
                break;
            }

            g_ConfiguratorInstance = new RpcConfigurator(serializetype, configSource, cachepath);
            *ppconfigurator = g_ConfiguratorInstance;
            rv = true;

            DebugPrintf(SV_LOG_ALWAYS, 
                "Configurator cx ip:%s port:%d.\n",
                GetCxIpAddress().c_str(), 
                localConfigurator.getHttp().port);
        }
        catch ( ContextualException& ce )
        {
            rv = false;
            g_ConfiguratorInstance = NULL;

            DebugPrintf(logLevel, 
                "Configurator instantiation failed cx ip:%s port:%d exception:%s.\n",
                GetCxIpAddress().c_str(),
                localConfigurator.getHttp().port, 
                ce.what());
        }
        catch ( ... )
        {
            rv = false;
            g_ConfiguratorInstance = NULL;

            DebugPrintf(logLevel, 
                "Configurator instantiation failed cx ip:%s port:%d exception:unknown.\n",
                GetCxIpAddress().c_str(),
                localConfigurator.getHttp().port);
        }

        if(!rv)
            break;

        try
        {
            g_ConfiguratorInstance ->GetCurrentSettings();

        }
        catch ( ContextualException& ce )
        {
            rv = false;
            g_ConfiguratorInstance->Stop();
            delete g_ConfiguratorInstance;
            g_ConfiguratorInstance = NULL;
            *ppconfigurator = NULL;

            DebugPrintf(logLevel, 
                "Configurator GetInitialSettings failed cx ip:%s port:%d exception:%s.\n",
                GetCxIpAddress().c_str(),
                localConfigurator.getHttp().port, 
                ce.what());
        }
        catch ( ... )
        {
            rv = false;
            g_ConfiguratorInstance->Stop();
            delete g_ConfiguratorInstance;
            g_ConfiguratorInstance = NULL;
            *ppconfigurator = NULL;

            DebugPrintf(logLevel, 
                "Configurator GetInitialSettings failed cx ip:%s port:%d exception:unknown.\n",
                GetCxIpAddress().c_str(),
                localConfigurator.getHttp().port);
        }

    } while(0);

    if(rv)
    {
        DebugPrintf(SV_LOG_DEBUG, "Configurator instantiated.\n");
    }

    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return rv;
}

/*
* FUNCTION NAME : InitializeConfigurator
*
* DESCRIPTION : create a singleton instance of the Configurator.
*
* INPUT PARAMETERS : 
*
* ip     - cx ip
* port   - cx port
* hostId - agent id
*
* OUTPUT PARAMETERS : 
*   configurator instance
*
* NOTES :
* 
*
* return value : true on success, false otherwise
*
*/

bool InitializeConfigurator(Configurator** ppconfigurator, 
                            std::string const& ip, 
                            int port, 
                            std::string const& hostId,
                            const SerializeType_t serializetype) 
{
    bool rv = false;
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    ACE_Guard<ACE_Mutex> guard( g_lock );

    do 
    {

        try
        {

            if(!ppconfigurator)
            {
                rv = false;
                break;
            }

            if(Configurator::instanceFlag)
            {
                // configurator was previously created
                // return the existing configurator
                // Configurator initialization (creation) shall
                // be done only once

                *ppconfigurator = g_ConfiguratorInstance;
                rv = true;
                break;
            }

            g_ConfiguratorInstance = new RpcConfigurator( serializetype, ip, port, hostId); 
            *ppconfigurator = g_ConfiguratorInstance;
            rv = true;

        }
        catch ( ContextualException& ce )
        {
            rv = false;
            g_ConfiguratorInstance = NULL;

            DebugPrintf(logLevel, 
                "Configurator instantiation failed cx ip:%s port:%d exception:%s.\n",
                ip.c_str(), port, ce.what());
        }
        catch ( ... )
        {
            rv = false;
            g_ConfiguratorInstance = NULL;

            DebugPrintf(logLevel, 
                "Configurator instantiation failed cx ip:%s port:%d exception:unknown.\n",
                ip.c_str(), port);
        }

        if(!rv)
            break;

        try
        {
            g_ConfiguratorInstance ->GetCurrentSettings();

        }
        catch ( ContextualException& ce )
        {
            rv = false;
            g_ConfiguratorInstance->Stop();
            delete g_ConfiguratorInstance;
            g_ConfiguratorInstance = NULL;
            *ppconfigurator = NULL;

            DebugPrintf(logLevel, 
                "Configurator instantiation failed cx ip:%s port:%d exception:%s.\n",
                ip.c_str(), port, ce.what());
        }
        catch ( ... )
        {
            rv = false;
            g_ConfiguratorInstance->Stop();
            delete g_ConfiguratorInstance;
            g_ConfiguratorInstance = NULL;
            *ppconfigurator = NULL;

            DebugPrintf(logLevel, 
                "Configurator GetInitialSettings failed cx ip:%s port:%d exception:unknown.\n",
                ip.c_str(), port);
        }

    } while (0);

    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return rv;
}

/*
* FUNCTION NAME : GetConfigurator
*
* DESCRIPTION : return previously created instance of the Configurator.
*
* INPUT PARAMETERS : none
*
*
* OUTPUT PARAMETERS : 
*   configurator instance
*
* NOTES :
* 
*
* return value : true on success, false otherwise
*
*/

bool GetConfigurator(Configurator** ppconfigurator)
{
    bool rv = false;
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);  

    ACE_Guard<ACE_Mutex> guard( g_lock );

    do
    {

        if(!ppconfigurator)
        {
            rv = false;
            break;
        }

        if(Configurator::instanceFlag)
        {

            // configurator was previously created
            // return the existing configurator
            // Configurator initialization (creation) shall
            // be done only once

            *ppconfigurator = g_ConfiguratorInstance;
            rv = true;
            break;

        } else
        {
            rv = false;
            break;
        }

    } while (0);

    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return rv;
}


/*
* FUNCTION NAME : makeSnapshotActive 
*
* DESCRIPTION :  wrapper over configurator call to make a snapshot instance ready
*                on arrival of a event.
*
* INPUT PARAMETERS : snapshotid - snapshot instance identifier
*
* OUTPUT PARAMETERS :  
*
* NOTES : 
*
* return value : true on success, false otherwise
*
*/
bool makeSnapshotActive(Configurator & configurator, const std::string & snapshotId) 
{
    bool rv = true;

    do
    {
        try
        {
            SV_UINT rc = configurator.getVxAgent().makeSnapshotActive(snapshotId);

            //
            // if activation request fails due to conflict
            //
            if(rc == 2) 
            {
                rv = false;
                DebugPrintf(logLevel, 
                    "\nmakeSnapshotActive call to cx failed with return code 2 for snapshot id %s.\n",
                    snapshotId.c_str());
                break;
            }

            // 
            // activation fails due to some other reason
            // 
            if(rc == 0) 
            {
                rv = false;
                DebugPrintf(logLevel, 
                    "\nmakeSnapshotActive call to cx failed with return code 0 for snapshot id %s.\n",
                    snapshotId.c_str());
                break;
            }

            rv = true;
        }
        catch ( ContextualException& ce )
        {
            rv = false;
            DebugPrintf(logLevel, 
                "\nmakeSnapshotActive call to cx for snapshot id %s failed with exception %s.\n",
                snapshotId.c_str(), ce.what());
        }
        catch ( ... )
        {
            rv = false;
            DebugPrintf(logLevel, 
                "\nmakeSnapshotActive call to cx for snapshot id %s failed with unknown exception.\n",
                snapshotId.c_str());
        }
    } while(0);

    return rv;
}

/*
* FUNCTION NAME : notifyCxOnSnapshotStatus 
*
* DESCRIPTION :  wrapper over configurator call to send snapshot status information
*
* INPUT PARAMETERS : snapId - snapshot instance identifier
*                    rest - todo: explanation pending?
*
* OUTPUT PARAMETERS :  
*
* NOTES : 
*
* return value : success/failure status from Cx 
*
*/
bool notifyCxOnSnapshotStatus(Configurator & configurator, const std::string & snapId, int timeval,const SV_ULONGLONG &VsnapId, const std::string &errMessage, int status) 
{
    bool rv = true;

    try
    {
        status = configurator.getVxAgent().notifyCxOnSnapshotStatus(snapId,timeval,VsnapId,errMessage,status);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxOnSnapshotStatus call to cx for snapshot id %s failed with exception %s.\n",
            snapId.c_str(), ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxOnSnapshotStatus call to cx for snapshot id %s failed with unknown exception.\n",
            snapId.c_str());
    }

    return rv;
}

/*
* FUNCTION NAME : notifyCxOnSnapshotProgress 
*
* DESCRIPTION :  wrapper over configurator call to send snapshot progress information
*
* INPUT PARAMETERS : snapId - snapshot instance identifier
*                    percentage - progress indicator
*                    rpoint - actual recovery point (currently unused)
*
* OUTPUT PARAMETERS :  
*
* NOTES : 
*
* return value : success/failure status from Cx 
*
*/
bool notifyCxOnSnapshotProgress(Configurator & configurator, const std::string &snapId, int percentage, int rpoint) 
{
    bool rv = true;

    try
    {
        rv = configurator.getVxAgent().notifyCxOnSnapshotProgress(snapId,percentage,rpoint);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxOnSnapshotProgress call to cx for snapshot id %s failed with exception %s.\n",
            snapId.c_str(), ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxOnSnapshotProgress call to cx for snapshot id %s failed with unknown exception.\n",
            snapId.c_str());
    }

    return rv;
}


/*
* FUNCTION NAME : notifyCxOnSnapshotCreation 
*
* DESCRIPTION :  wrapper over configurator call to send snapshot status information
*
* INPUT PARAMETERS : vsnapInfo - Vsnap Information to be added to database
*                    
*
* OUTPUT PARAMETERS :  results - list of true/false values representing the sucess/failure of vsnap updation at CX
*
* NOTES : 
*
* return value : success/failure status from Cx 
*
*/
bool notifyCxOnSnapshotCreation(Configurator & configurator,std::vector<VsnapPersistInfo> vsnapInfo, std::vector<bool>& results) 
{
    bool rv = true;
    std::vector<VsnapPersistInfo>::iterator it = vsnapInfo.begin();
    for(; it != vsnapInfo.end(); ++it)
    {
        convertToCxFormattedPersistInfo((*it));
    }
    try
    {
        results = configurator.getVxAgent().notifyCxOnSnapshotCreation(vsnapInfo);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxOnSnapshotCreation call to cx for failed with exception %s.\n",
            ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxOnSnapshotCreation call to cx for  failed with unknown exception.\n"
            );
    }

    return rv;
}

bool notifyCxOnSnapshotDeletion(Configurator & configurator,std::vector<VsnapDeleteInfo> vsnapInfo, std::vector<bool>& results)
{
    bool rv = true;
    std::vector<VsnapDeleteInfo>::iterator it = vsnapInfo.begin();
    for(; it != vsnapInfo.end(); ++it)
    {
        convertToCxFormattedVsnapDeleteInfo((*it));
    }
    try
    {
        results = configurator.getVxAgent().notifyCxOnSnapshotDeletion(vsnapInfo);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,
            "\nnotifyCxOnSnapshotDeletion call to cx for failed with exception %s.\n",
            ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel,
            "\nnotifyCxOnSnapshotDeletion call to cx for  failed with unknown exception.\n"
            );
    }

    return rv;
}
bool notifyCxDiffsDrained(Configurator & configurator, const std::string &devname, const SV_ULONGLONG& bytesAppliedPerSecond)
{
    bool rv = true;

    try
    {
        SV_INT rc = configurator.getVxAgent().notifyCxDiffsDrained(devname, bytesAppliedPerSecond);
        if( 0 != rc)
        {
            rv = false;
            DebugPrintf(logLevel, 
                "\nnotifyCxDiffsDrained call to cx for device %s failed with return code %d .\n",
                devname.c_str(), rc);
        }
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxDiffsDrained call to cx for device %s failed with exception %s.\n",
            devname.c_str(), ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nnotifyCxDiffsDrained call to cx for device %s failed with unknown exception.\n",
            devname.c_str());
    }

    return rv;
}


/*
* FUNCTION NAME : deleteVirtualSnapshot 
*
* DESCRIPTION :  wrapper over configurator call to informa vsnap deletion
*
* INPUT PARAMETERS : targetVolume - target volume name
*                    rest - todo: explanation pending?
*
* OUTPUT PARAMETERS :  
*
* NOTES : 
*
* return value : success/failure status from Cx 
*
*/
bool deleteVirtualSnapshot(Configurator & configurator, const std::string & targetVolume,unsigned long long vsnapid , int status, const std::string & message) 
{
    bool rv = true;

    try
    {
        std::string cxVolName = targetVolume;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);

        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        rv = configurator.getVxAgent().deleteVirtualSnapshot( cxVolName,vsnapid,status, message);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\ndeleteVirtualSnapshot call to cx for snapshot id " ULLSPEC " failed with exception %s.\n",
            vsnapid, ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\ndeleteVirtualSnapshot call to cx for snapshot id " ULLSPEC " failed with unknown exception.\n",
            vsnapid);
    }

    return rv;
}
// Bug #6298
/*
* FUNCTION NAME : updateReplicationStateStatus
*
* DESCRIPTION :  wrapper over configurator call to update the replication pair status(to go ahead deleting the rep. pair as the 
*				 vsnaps associated with the target are deleted)
*
* INPUT PARAMETERS : deviceName - the target volume of rep. pair for which the status is being reported
*                    state - the state being reported( CLEANUP_DONE on success/ CLEANUP_FAILED on failure)
*                    
*
* OUTPUT PARAMETERS :  None
*
* NOTES : None
*
* return value : success/failure status - failure in case of exception in reporting to cx
*
*/
bool updateReplicationStateStatus(Configurator & configurator, const std::string& deviceName, VOLUME_SETTINGS::PAIR_STATE state)
{
    bool rv = true;
    std::string cxVolName;
    try
    {
        cxVolName = deviceName;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        rv = configurator.getVxAgent().updateReplicationStateStatus( cxVolName, state );
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,
            "\nupdateReplicationStateStatus call to cx for device %s failed with exception %s.\n",
            cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,
            "\nupdateReplicationStateStatus call to cx for device %s failed with unknown exception.\n",
            cxVolName.c_str());
    }
    return rv;
}


bool updateVolumeAttribute(Configurator & configurator, NOTIFY_TYPE notifyType,const std::string & deviceName,VOLUME_STATE volumeState,const std::string & mountPoint, bool & status)
{

    bool rv = true;

    try
    {
        std::string cxVolName = deviceName;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        status = configurator.getVxAgent().updateVolumeAttribute(notifyType,cxVolName,volumeState,mountPoint);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nupdateVolumeAttribute call to cx for device %s failed with exception %s.\n",
            deviceName.c_str(), ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nupdateVolumeAttribute call to cx for device %s failed with unknown exception.\n",
            deviceName.c_str());
    }

    return rv;

}

int getCurrentVolumeAttribute(Configurator & configurator, const std::string & deviceName)
{
    CHANGEFSTAGOP failedStatus = OPERATION_FAILED;
    int visibilityStatus = failedStatus;

    try
    {
        std::string cxVolName = deviceName;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);

        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        visibilityStatus = configurator.getVxAgent().getCurrentVolumeAttribute(cxVolName);
    }
    catch ( ContextualException& ce )
    {
        visibilityStatus = failedStatus;
        DebugPrintf(logLevel, 
            "\ngetCurrentVolumeAttribute call to cx for device %s failed with exception %s.\n",
            deviceName.c_str(), ce.what());
    }
    catch ( ... )
    {
        visibilityStatus = failedStatus;
        DebugPrintf(logLevel, 
            "\ngetCurrentVolumeAttribute call to cx for device %s failed with unknown exception.\n",
            deviceName.c_str());
    }

    return visibilityStatus;
}

bool setTargetResyncRequired(Configurator & configurator, const std::string & deviceName, bool & status, const std::string& errStr,
    const ResyncReasonStamp &resyncReasonStamp, long errorcode)
{
    bool rv = true;

    try
    {
        std::string cxVolName = deviceName;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);

        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

		status = configurator.getVxAgent().setTargetResyncRequired(cxVolName, errStr, resyncReasonStamp, errorcode);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nsetTargetResyncRequired call to cx for device %s failed with exception %s.\n",
            deviceName.c_str(), ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, 
            "\nsetTargetResyncRequired call to cx for device %s failed with unknown exception.\n",
            deviceName.c_str());
    }

    return rv;
}

bool getSourceRawSize(Configurator & configurator, const std::string & deviceName, SV_ULONGLONG & capacity)
{
    bool rv = true;

    std::string cxVolName = deviceName;
    FormatVolumeNameForCxReporting(cxVolName);
    FirstCharToUpperForWindows(cxVolName);

    if (IsReportingRealNameToCx())
    {
        /* Then get the device name and send to CX */
        GetDeviceNameFromSymLink(cxVolName);
    }

    capacity = configurator.getVxAgent().getSourceRawSize(cxVolName);
    if(!capacity)
        rv = false;

    return rv;
}

bool getSourceCapacity(Configurator & configurator, const std::string & deviceName, SV_ULONGLONG & capacity)
{
    bool rv = true;

    std::string cxVolName = deviceName;
    FormatVolumeNameForCxReporting(cxVolName);
    FirstCharToUpperForWindows(cxVolName);

    if (IsReportingRealNameToCx())
    {
        /* Then get the device name and send to CX */
        GetDeviceNameFromSymLink(cxVolName);
    }

    capacity = configurator.getVxAgent().getSourceCapacity(cxVolName);
    if(!capacity)
        rv = false;

    return rv;
}


/*
* FUNCTION NAME : setIgnoreCxErrors
*
* DESCRIPTION :  set the flag to ignore cx errors
*
* INPUT PARAMETERS : status - true or false
*
* OUTPUT PARAMETERS :  None
*
* NOTES : 
*
* return value : None
*
*/

void setIgnoreCxErrors(bool status)
{
    ignoreCxErrors = status;
    if(ignoreCxErrors)
    {
        logLevel = SV_LOG_DEBUG;
    }
}

/*
* FUNCTION NAME : updatePendingDataInfo
*
* DESCRIPTION :  sends the pending data size of target volumes contained in pendingDataInfo
*
* INPUT PARAMETERS : pendingDataInfo
*
* OUTPUT PARAMETERS :  None
*
* NOTES : 
*
* return value : true if successeds false otherwise
*
*/
bool updatePendingDataInfo(Configurator & configurator, const std::map<std::string,SV_ULONGLONG>& pendingDataInfo)
{
    bool rv = true;

    try
    {
        rv = configurator.getVxAgent().updatePendingDataInfo(pendingDataInfo);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\nupdatePendingDataInfo call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\nupdatePendingDataInfo call to cx failed with unknown exception.\n");
    }

    return rv;
}

/*
* FUNCTION NAME : updateCleanUpActionStatus
*
* DESCRIPTION :  wrapper over configurator call to update the clean up status(to go ahead deleting the rep. pair as the 
*				 clean up associated with the target are done)
*
* INPUT PARAMETERS : deviceName - the target volume of rep. pair for which the status is being reported
*                    cleanupstr - the state, error message for 5 clean up status(Pending directory,vsnap, retaintion log, unlock volume) being reported
*                    
*
* OUTPUT PARAMETERS :  None
*
* NOTES : None
*
* return value : success/failure status - failure in case of exception in reporting to cx
*
*/
bool updateCleanUpActionStatus(Configurator & configurator, const std::string& deviceName, const std::string & cleanupstr)
{
    bool rv = true;
    std::string cxVolName;
    try
    {
        cxVolName = deviceName;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        DebugPrintf(SV_LOG_DEBUG, "@LINE %d in FILE %s, cxVolName = %s\n", LINE_NO, FILE_NAME, cxVolName.c_str());
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        rv = configurator.getVxAgent().updateCleanUpActionStatus( cxVolName, cleanupstr );
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,"\nupdateCleanUpActionStatus call to cx for device %s failed with exception %s.\n",cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,"\nupdateCleanUpActionStatus call to cx for device %s failed with unknown exception.\n",cxVolName.c_str());
    }
    return rv;
}

/*
* FUNCTION NAME : updateRestartResyncCleanupStatus
*
* DESCRIPTION :  wrapper over configurator call to update the clean up status on restart resync
*
* INPUT PARAMETERS : deviceName - the target volume of rep. pair for which the status is being reported
*                    success - whether the cleanup operation succeded or failed on target
*                    
*
* OUTPUT PARAMETERS :  None
*
* NOTES : None
*
* return value : success/failure status - failure in case of exception in reporting to cx
*
*/
bool updateRestartResyncCleanupStatus(Configurator & configurator, const std::string& deviceName, bool& success, const std::string& err_message)
{
    bool rv = true;
    std::string cxVolName;
    try
    {
        cxVolName = deviceName;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        DebugPrintf(SV_LOG_DEBUG, "@LINE %d in FILE %s, cxVolName = %s\n", LINE_NO, FILE_NAME, cxVolName.c_str());
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        rv = configurator.getVxAgent().updateRestartResyncCleanupStatus( cxVolName, success, err_message );
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,"\nupdateRestartResyncCleanupStatus call to cx for device %s failed with exception %s.\n",cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,"\nupdateRestartResyncCleanupStatus call to cx for device %s failed with unknown exception.\n",cxVolName.c_str());
    }
    return rv;
}

bool sendCacheCleanupStatus(Configurator & configurator, const std::string & devicename, bool status, const std::string & info)
{

    bool rv = true;
    std::string cxVolName;

    try
    {
        cxVolName = devicename;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);

        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        std::stringstream cache_cleanup_status;
        VOLUME_SETTINGS::PAIR_STATE state = (status)? VOLUME_SETTINGS::CACHE_CLEANUP_COMPLETE:VOLUME_SETTINGS::CACHE_CLEANUP_FAILED;
        cache_cleanup_status << "cache_dir_del=yes;"
            << "cache_dir_del_status="
            << state << ";"
            << "cache_dir_del_message="
            <<  info << ";";

        rv = configurator.getVxAgent().updateCleanUpActionStatus( cxVolName, cache_cleanup_status.str());
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,
            "\nupdateCleanUpActionStatus call to cx for device %s failed with exception %s.\n",
            cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,
            "\nupdateCleanUpActionStatus call to cx for device %s failed with unknown exception.\n",
            cxVolName.c_str());
    }

    return rv;
}

bool registerClusterInfo(Configurator & configurator, const std::string & action, const ClusterInfo & clusterInfo )
{
    bool rv = true;

    try
    {
        configurator.getVxAgent().registerClusterInfo(action, clusterInfo);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\nregisterClusterInfo call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\nregisterClusterInfo call to cx failed with unknown exception.\n");
    }

    return rv;
}

bool getResyncStartTimeStamp(Configurator & configurator, const std::string & volname, const std::string & jobId, ResyncTimeSettings& rtsettings, const std::string &hostType)
{
    bool rv = true;

    try
    {
        rtsettings = configurator.getVxAgent().getResyncStartTimeStamp(volname, jobId, hostType);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetResyncStartTimeStamp call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetResyncStartTimeStamp call to cx failed with unknown exception.\n");
    }

    return rv;
}
bool getTargetReplicationJobId(Configurator & configurator,  std::string deviceName, std::string& jid )
{
    bool rv = true;

    try
    {
        jid = configurator.getVxAgent().getTargetReplicationJobId(deviceName);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetTargetReplicationJobId call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetTargetReplicationJobId call to cx failed with unknown exception.\n");
    }

    return rv;
}
bool getResyncEndTimeStamp(Configurator & configurator, const std::string & volname, const std::string & jobId, ResyncTimeSettings& rtsettings, const std::string &hostType)
{
    bool rv = true;

    try
    {
        rtsettings = configurator.getVxAgent().getResyncEndTimeStamp(volname, jobId, hostType);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetResyncEndTimeStamp call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetResyncEndTimeStamp call to cx failed with unknown exception.\n");
    }

    return rv;
}
bool getVolumeCheckpoint(Configurator & configurator,  std::string const & drivename, JOB_ID_OFFSET& jidoffset )
{
    bool rv = true;

    try
    {
        jidoffset = configurator.getVxAgent().getVolumeCheckpoint(drivename);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetVolumeCheckpoint call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetVolumeCheckpoint call to cx failed with unknown exception.\n");
    }

    return rv;
}
bool getVsnapRemountVolumes(Configurator & configurator, VsnapRemountVolumes& remountvols)
{
    bool rv = true;

    try
    {
        remountvols = configurator.getVxAgent().getVsnapRemountVolumes();
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetVsnapRemountVolumes call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\ngetVsnapRemountVolumes call to cx failed with unknown exception.\n");
    }

    return rv;
}


bool sendEndQuasiStateRequest(Configurator & configurator, const std::string & volname, int& status)
{
    bool rv = true;

    try
    {
        status = configurator.getVxAgent().sendEndQuasiStateRequest(volname);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel, "\nsendEndQuasiStateRequest call to cx failed with exception %s.\n", ce.what());
    }
    catch ( ... )
    {
        rv = false;
        DebugPrintf(logLevel, "\nsendEndQuasiStateRequest call to cx failed with unknown exception.\n");
    }

    return rv;
}

/*
* FUNCTION NAME : NotifyMaintenanceActionStatus
*
* DESCRIPTION :  wrapper over configurator call to update the pause replication status(to go ahead pause maintenace or 
*				 pause pending activity is done for	the rep. pair as the 
*				 pause maintenance task associated with the target are done)
*
* INPUT PARAMETERS : deviceName - the target volume of rep. pair for which the status is being reported
*                    cleanupstr - the state, error message for 5 clean up status(Pending directory,vsnap, retaintion log, unlock volume) being reported
*                    
*
* OUTPUT PARAMETERS :  None
*
* NOTES : None
*
* return value : success/failure status - failure in case of exception in reporting to cx
*
*/
bool NotifyMaintenanceActionStatus(Configurator & configurator, const std::string & devicename, int hosttype, const std::string & respstr)
{
    bool rv = true;
    std::string cxVolName;
    try
    {
        cxVolName = devicename;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        DebugPrintf(SV_LOG_DEBUG, "@LINE %d in FILE %s, cxVolName = %s\n", LINE_NO, FILE_NAME, cxVolName.c_str());
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        rv = configurator.getVxAgent().setPauseReplicationStatus( cxVolName,hosttype,respstr);
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,"\nNotifyMaintenanceActionStatus call to cx for device %s failed with exception %s.\n",cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,"\nNotifyMaintenanceActionStatus call to cx for device %s failed with unknown exception.\n",cxVolName.c_str());
    }
    return rv;
}


void convertToCxFormattedPersistInfo(VsnapPersistInfo & vsnap)
{
    if(!vsnap.mountpoint.empty())
        FormatVolumeNameForCxReporting(vsnap.mountpoint);
    if(!vsnap.devicename.empty())
        FormatVolumeNameForCxReporting(vsnap.devicename);
    if(!vsnap.target.empty())
        FormatVolumeNameForCxReporting(vsnap.target);

}

void convertToCxFormattedVsnapDeleteInfo(VsnapDeleteInfo & vsnap)
{
    FormatVolumeNameForCxReporting(vsnap.target_device);
    FormatVolumeNameForCxReporting(vsnap.vsnap_device);
}

bool IsVsnapDriverAvailable()
{
    Configurator * configurator;
    bool rv = true;
    if(GetConfigurator(&configurator))
    {
        rv = configurator->getVxAgent().IsVsnapDriverAvailable();
    }
    else
    {
        // assign the default expected values
        rv = true;
    }
    return rv;
}

bool IsVolpackDriverAvailable()
{
    Configurator * configurator;
    bool rv = true;
    if(GetConfigurator(&configurator))
    {
        rv = configurator->getVxAgent().IsVolpackDriverAvailable();
    }
    else
    {
        // assign the default expected values
        rv = false;
    }
    return rv;
}


void printVolVgs(VolToVgs_t &voltovgs)
{
    for (VolToVgsIter_t cit = voltovgs.begin(); cit != voltovgs.end(); cit++)
    {
        DebugPrintf(SV_LOG_DEBUG, "====\n");
        DebugPrintf(SV_LOG_DEBUG, "volume: %s\n", cit->first);
        printVgsPtrVec(cit->second);
        DebugPrintf(SV_LOG_DEBUG, "====\n");
    }
}


void printVgsPtrVec(VgsPtrs_t &vgs)
{
    for (VgsPtrsIter_t vit = vgs.begin(); vit != vgs.end(); vit++)
    {
        VOLUME_GROUP_SETTINGS *pvgs = *vit;
        VOLUME_GROUP_SETTINGS &vgs = *pvgs;
        DebugPrintf(SV_LOG_DEBUG, "volumegroup id: %d\n", vgs.id);
        for (VOLUME_GROUP_SETTINGS::volumes_iterator volit = vgs.volumes.begin(); volit != vgs.volumes.end(); volit++)
        {
            VOLUME_SETTINGS &vs = volit->second;
            DebugPrintf(SV_LOG_DEBUG, "device name: %s\n", volit->first.c_str());
            DebugPrintf(SV_LOG_DEBUG, "host id: %s\n", vs.hostId.c_str());
        }
    }
}

void Object::Print(const SV_LOG_LEVEL LogLevel)
{
    PrintAttributes(m_Attributes, LogLevel);
}


void Object::Reset(void)
{
    m_Attributes.clear();
}


void PrintAttributes(const Attributes_t &attributes, const SV_LOG_LEVEL LogLevel)
{
	DebugPrintf(LogLevel, "Attributes (name --> value):\n");
	for (ConstAttributesIter_t it = attributes.begin(); it != attributes.end(); it++)
	{
		DebugPrintf(LogLevel, "%s --> %s\n", it->first.c_str(), it->second.c_str());
	}
}


std::ostream& operator<< (std::ostream& out, const Objects_t &objs)
{
    for (ConstObjectsIter_t oit = objs.begin(); oit != objs.end(); oit++)
    {
        out << *oit;
    }

    return out;
}


std::ostream& operator<< (std::ostream& out, const Object &o)
{
    out << "object:\n";
    out << o.m_Attributes;

    return out;
}


std::ostream& operator<< (std::ostream& out, const Attributes_t &attrs)
{
	out << "Attributes (name --> value):\n";
	for (ConstAttributesIter_t it = attrs.begin(); it != attrs.end(); it++)
	{
        out << it->first << " --> " << it->second << '\n';
	}

    return out;
}


HOST_VOLUME_GROUP_SETTINGS::volumeGroups_t GetEndPointVolumeSettings(HOST_VOLUME_GROUP_SETTINGS& hostVolumeGroupSettings,std::list<VOLUME_SETTINGS> endPointVolSettings)
{
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    bool bFound = false;
    HOST_VOLUME_GROUP_SETTINGS::volumeGroups_t endPointVolumeGroupSettings;
    VOLUME_GROUP_SETTINGS::volumes_iterator volumeIter;
    VOLUME_GROUP_SETTINGS::volumes_iterator volumeEnd;
    HOST_VOLUME_GROUP_SETTINGS::volumeGroups_iterator volumeGroupIter = hostVolumeGroupSettings.volumeGroups.begin();
    HOST_VOLUME_GROUP_SETTINGS::volumeGroups_iterator volumeGroupEnd =  hostVolumeGroupSettings.volumeGroups.end();
    for (; volumeGroupIter != volumeGroupEnd; ++volumeGroupIter) 
    {
        volumeIter = (*volumeGroupIter).volumes.begin();
        volumeEnd = (*volumeGroupIter).volumes.end();
        bFound = false;
        if( (TARGET == (*volumeGroupIter).direction))
        {
            for (; volumeIter != volumeEnd && (!bFound); ++volumeIter) 
            {
                std::string deviceName = (*volumeIter).second.deviceName;
                std::string hostId = (*volumeIter).second.hostId;;
                VOLUME_SETTINGS::endpoints_iterator endPointsIter = endPointVolSettings.begin();
                VOLUME_SETTINGS::endpoints_iterator endPointsIterEnd = endPointVolSettings.end();
                while(endPointsIter != endPointsIterEnd)
                {
                    if((deviceName == endPointsIter->deviceName) &&
                        (hostId.compare(endPointsIter->hostId) == 0))   
                    {
                        DebugPrintf(SV_LOG_DEBUG,"Device Name %s EndPoint Device Name %s\n",deviceName.c_str(),endPointsIter->deviceName.c_str());
                        endPointVolumeGroupSettings.push_back((*volumeGroupIter));
                        bFound = true;
                        break;
                    }                    
                    endPointsIter++;
                }
            }
        }
    }
    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return endPointVolumeGroupSettings;
}

bool sendFlushAndHoldWritesPendingStatusToCx(Configurator & configurator,std::string volumename,bool status, int error_num, std::string errmsg)
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    bool rv = true;
    std::string cxVolName;
    try
    {
        cxVolName = volumename;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

		DebugPrintf(SV_LOG_DEBUG, "%s, cxVolName = %s, status = %d, error_msg = %s, error_num = %d\n", FUNCTION_NAME, cxVolName.c_str(), rv, errmsg.c_str(), error_num);
		if(configurator.getVxAgent().updateFlushAndHoldWritesPendingStatus(cxVolName,status,errmsg,error_num) == 1)
		{
			rv = false;
		}
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,"sendFlushAndHoldWritesPendingStatusToCx call to cx for device %s failed with exception %s.\n",cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,"sendFlushAndHoldWritesPendingStatusToCx call to cx for device %s failed with unknown exception.\n",cxVolName.c_str());
    }
	DebugPrintf(SV_LOG_DEBUG,"LEAVING %s\n",FUNCTION_NAME);
    return rv;

}
bool sendFlushAndHoldResumePendingStatusToCx(Configurator & configurator,std::string volumename,bool status, int error_num, std::string errmsg)
{
    bool rv = true;
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    std::string cxVolName;
    try
    {
        cxVolName = volumename;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }
		DebugPrintf(SV_LOG_DEBUG, "%s, cxVolName = %s, status = %d, error_msg = %s, error_num = %d\n", FUNCTION_NAME, cxVolName.c_str(),rv,errmsg.c_str(),error_num);
		if(configurator.getVxAgent().updateFlushAndHoldResumePendingStatus(cxVolName,status,errmsg,error_num) == 1)
		{
			rv = false;
		}
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,"sendFlushAndHoldResumePendingStatusToCx call to cx for device %s failed with exception %s.\n",cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,"sendFlushAndHoldResumePendingStatusToCx call to cx for device %s failed with unknown exception.\n",cxVolName.c_str());
    }
	DebugPrintf(SV_LOG_DEBUG,"LEAVING %s\n",FUNCTION_NAME);
    return rv;
}

bool getFlushAndHoldRequestSettings(Configurator & configurator,std::string volumename,FLUSH_AND_HOLD_REQUEST & flushAndHoldRequestSettings)
{
    bool rv = true;
    std::string cxVolName;
    try
    {
        cxVolName = volumename;
        FormatVolumeNameForCxReporting(cxVolName);
        FirstCharToUpperForWindows(cxVolName);
        if (IsReportingRealNameToCx())
        {
            /* Then get the device name and send to CX */
            GetDeviceNameFromSymLink(cxVolName);
        }

        DebugPrintf(SV_LOG_DEBUG, "%s, cxVolName = %s\n", FUNCTION_NAME, cxVolName.c_str());

		flushAndHoldRequestSettings = configurator.getVxAgent().getFlushAndHoldRequestSettings(cxVolName);
    }
    catch( ContextualException& ce )
    {
        rv = false;
        DebugPrintf(logLevel,"getFlushAndHoldRequestSettings call to cx for device %s failed with exception %s.\n",cxVolName.c_str(), ce.what());
    }
    catch( ... )
    {
        rv = false;
        DebugPrintf(logLevel,"getFlushAndHoldRequestSettings call to cx for device %s failed with unknown exception.\n",cxVolName.c_str());
    }
    return rv;

}


bool cdpStopReplication(Configurator & configurator, const std::string &volname, const std::string &cleanupaction) 
{
    bool rv = true;

    try
    {
        rv = configurator.getVxAgent().cdpStopReplication(volname,cleanupaction);
    }
    catch ( ContextualException& ce )
    {
        rv = false;
		DebugPrintf(logLevel,
			"\nNote: Replication pair deletion request for %s failed with exception %s\n. It will be deleted when communication with Central Management Server is restored.\n", volname.c_str(), ce.what());
    }
    catch ( ... )
    {
        rv = false;
		DebugPrintf(logLevel,
			"\nNote: Replication pair deletion request for %s did not complete and will be tried again when communication with Central Management Server is restored\n\n",volname.c_str() );
    }

    return rv;
}

bool SendAlertToRcm(SV_ALERT_TYPE AlertType, SV_ALERT_MODULE AlertingModule, const InmAlert &Alert)
{
    return true;
}

bool SendAlertToCx(SV_ALERT_TYPE AlertType, SV_ALERT_MODULE AlertingModule, const InmAlert &Alert)
{
    LocalConfigurator localConfigurator;
    if (localConfigurator.isMobilityAgent() &&
        (localConfigurator.IsAzureToAzureReplication() ||
            boost::iequals(localConfigurator.getCSType(), CSTYPE_CSPRIME)))
    {
        DebugPrintf(SV_LOG_DEBUG, "SendAlertToCx: Control plane is RCM.\n");
        return SendAlertToRcm(AlertType, AlertingModule, Alert);
    }
    boost::mutex::scoped_lock guard(g_AlertMutex);
	bool rv = false;
    static LastAlertDetails s_La;

    Configurator* TheConfigurator = NULL;
    if(!GetConfigurator(&TheConfigurator))
    {
        DebugPrintf(SV_LOG_DEBUG, "Error obtaining configurator %s\t%d\n", FUNCTION_NAME, LINE_NO);
        DebugPrintf(SV_LOG_ERROR,"FAILED:sendAlertToCx call failed.\n");
        return rv;
    }

	time_t ltime;
	struct tm *today;

	std::string Agent_type; /* To hold the agent type */

	time( &ltime );
    time_t ltimecopy = ltime;
    std::stringstream alertwithtypeandmodule;
    const char SEP = '@';
    alertwithtypeandmodule << Alert.GetID() << SEP << cxArg(Alert.GetParameters()) << SEP << AlertType << SEP << AlertingModule;
    
    if (alertwithtypeandmodule.str() == s_La.GetAlert()) {
        if ((ltimecopy-s_La.GetTime()) <= TheConfigurator->getVxAgent().getRepeatingAlertIntervalInSeconds()) {
            DebugPrintf(SV_LOG_DEBUG, "Alert %s is already recorded (with same parameters) with in repeating alert interval. Hence not recording again.\n", alertwithtypeandmodule.str().c_str());
            rv = true;
            return rv;
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "Recording alert %s\n", alertwithtypeandmodule.str().c_str());
	today = gmtime( &ltime );

	char szGmtTimeBuff[100]; /* To hold the time in  buffer form */

	// initialize 100 bytes of the character string to 0
	memset(szGmtTimeBuff,0,100);

	inm_sprintf_s(szGmtTimeBuff, ARRAYSIZE(szGmtTimeBuff), "(20%02d-%02d-%02d %02d:%02d:%02d):",
		today->tm_year - 100,
		today->tm_mon + 1,
		today->tm_mday,		
		today->tm_hour,
		today->tm_min,
		today->tm_sec
		);

	if(GetLoggerAgentType() == FxAgentLogger)
		Agent_type="FX";
	else
		Agent_type="VX";

	try
	{
        if(TheConfigurator->getVxAgent().sendAlertToCx(szGmtTimeBuff, "ALERT", Agent_type,
                                                       AlertType, AlertingModule, Alert))
        {
            s_La.SetDetails(alertwithtypeandmodule.str(), ltimecopy);
            rv = true; 
        }
	}catch(...) {
		DebugPrintf(SV_LOG_ERROR,"FAILED:sendAlertToCx call failed for alert id, parameters and module: %s\n", alertwithtypeandmodule.str().c_str());
	}

	return (rv);
}
