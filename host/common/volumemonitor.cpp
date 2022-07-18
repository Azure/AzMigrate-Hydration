// Copyright (c) 2005 InMage.
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use,
// disclosure, or reproduction is prohibited except as permitted
// by express written license agreement with InMage.
// 
// File       : volumemonitor.cpp
// 
// Description: 
//
#ifndef INITGUID
#define INITGUID
#endif

#include <windows.h>
#include <winioctl.h>
#include <dbt.h>
#include <iostream>
#include <ioevent.h>
#include <sstream>
#include <process.h>
#include <iomanip>
#include <devguid.h>
#include <exception>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "hostagenthelpers_ported.h"
#include "portablehelpersmajor.h"
#include "volumemonitor.h"

Lockable VolumeMonitor::SyncLock;

std::string const VolumeDeviceInterfaceName("VolumeDeviceInterface");
std::string const UsbDeviceInterfaceName("UsbDeviceInterface");

// ----------------------------------------------------------------------------
// trace support
// ----------------------------------------------------------------------------
// #define SIMPLE_TRACE_ON
#ifdef SIMPLE_TRACE_ON
#define SIMPLE_TRACE(msg) \
do { \
    std::ostringstream o; \
    o << msg; \
    OutputDebugString(o.str().c_str()); \
} while(0);
#else
#define SIMPLE_TRACE(msg)
#endif

// -------------------------------------------
// VolumeMonitor::SimpleLock
// -------------------------------------------
/*
VolumeMonitor::SimpleLock::SimpleLock()
{
    m_Lock = CreateMutex(0, false, 0);
}

VolumeMonitor::SimpleLock::~SimpleLock()
{
    if (0 != m_Lock) {
        CloseHandle(m_Lock);
    }
}

bool VolumeMonitor::SimpleLock::Lock() 
{   
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
 
    if (WAIT_OBJECT_0 == WaitForSingleObject(m_Lock, INFINITE)) {
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return true;
    } else {        
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return false;
    }    
}

void VolumeMonitor::SimpleLock::Unlock() {
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    ReleaseMutex(m_Lock);
    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
}
*/
// -------------------------------------------
// VolumeMonitor::SimpleEvent
// -------------------------------------------
VolumeMonitor::SimpleEvent::SimpleEvent()
{
    m_Event = CreateEvent(0, FALSE, FALSE, "SimpleEvent");
}

VolumeMonitor::SimpleEvent::~SimpleEvent()
{
    if (0 != m_Event) {
        CloseHandle(m_Event);
    }
}

bool VolumeMonitor::SimpleEvent::Wait() 
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    bool ok = false;
    if (0 != m_Event) {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_Event, INFINITE)) {
            ok = true;
        } else {        
            ok = false;
        }
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return ok;
}

void VolumeMonitor::SimpleEvent::Set() {
    SetEvent(m_Event);
}

// -------------------------------------------
// VolumeMonitor
// -------------------------------------------
VolumeMonitor::~VolumeMonitor()
{
    Stop();    

    if (0 != m_Window) {
        DestroyWindow(m_Window);
    }

    if (0 != m_Atom) {
        UnregisterClass((LPCTSTR)m_Atom, GetModuleHandle(NULL));
    }

    if (0 != m_Thread) {
        CloseHandle(m_Thread);
    }
}

bool VolumeMonitor::Register()
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    bool ok = false;
    if (m_Registered) {
        ok = true;
    } else {
        if (ERROR_SUCCESS == CreateNotificationWindow()) {
            ok = Register(m_Window, DEVICE_NOTIFY_WINDOW_HANDLE);
            m_ClientSignal = true;
        }
    }

    // let caller know registration is done. 
    // neded in case this is being done in a seperate thread
    Signal(); 

	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return ok;
}

bool VolumeMonitor::Register(HANDLE h, DWORD handleType)
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

/*    if (!Lock()) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Register Lock failed: %d\n", GetLastError());
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return false;
    }  
*/
    AutoLock LockSection(SyncLock);

//    boost::shared_ptr<void> guard((void*)NULL, boost::bind<void>(&VolumeMonitor::Unlock, this));

    bool ok = true;
    
    m_RegisterHandle = h;
    m_RegisterHandleType = handleType;    
       
    // register for volume interface Monitor
    DEV_BROADCAST_DEVICEINTERFACE interfaceFilter;    
    ZeroMemory(&interfaceFilter, sizeof(interfaceFilter));
    interfaceFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    interfaceFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    interfaceFilter.dbcc_classguid  = GUID_DEVINTERFACE_VOLUME;
    HDEVNOTIFY notify = RegisterDeviceNotification(m_RegisterHandle, &interfaceFilter, m_RegisterHandleType);
    if (0 == notify) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Register RegisterDeviceNotification DBT_DEVTYP_DEVICEINTERFACE GUID_DEVINTERFACE_VOLUME failed: %d\n", GetLastError());
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return false; 
    }            

    m_Monitors.insert(MONITOR_PAIR(notify, VolumeState(VolumeDeviceInterfaceName, GUID_IO_VOLUME_MOUNT)));

    // now set up for usb

    ZeroMemory(&interfaceFilter, sizeof(interfaceFilter));
    interfaceFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    interfaceFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    interfaceFilter.dbcc_classguid  = GUID_DEVCLASS_USB;
    notify = RegisterDeviceNotification(m_RegisterHandle, &interfaceFilter, m_RegisterHandleType);
    if (0 == notify) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Register RegisterDeviceNotification DBT_DEVTYP_DEVICEINTERFACE GUID_DEVCLASS_USB failed: %d\n", GetLastError());
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return false; 
    }        

    m_Monitors.insert(MONITOR_PAIR(notify, VolumeState(UsbDeviceInterfaceName, GUID_IO_VOLUME_MOUNT)));

//    Unlock();

    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return RegisterDrives(); 
}

bool VolumeMonitor::ReregisterAll()
{
 	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
   // clear all registations and get new notification objects
    m_Registered = false;
    UnregisterAll();
    m_Monitors.clear();
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return Register(m_RegisterHandle, m_RegisterHandleType);
}

bool VolumeMonitor::UnregisterAll()
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

//    Lock();    
    AutoLock LockSection(SyncLock);
//    boost::shared_ptr<void> guard((void*)NULL, boost::bind<void>(&VolumeMonitor::Unlock, this));     

    bool ok = true;
    for (MONITORS::iterator iter = m_Monitors.begin(); iter != m_Monitors.end(); ++iter) {
        if (!UnregisterDeviceNotification((*iter).first)) {
            DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::UnregisterAll UnregisterDeviceNotification for device %s failed: %d\n", (*iter).second.Device().c_str(), GetLastError());
            ok = false;
        }        
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return ok;
}

void VolumeMonitor::UnregisterDriveLetter(char drive)
{    
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    SIMPLE_TRACE("VolumeMonitor::UnregisterDrive\n");
/*    if (!Lock()) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::UnregisterDriveLetter Lock failed: %d\n", GetLastError());
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return;
    }
*/
    AutoLock LockSection(SyncLock);

    char guid[128];

    char name[4] = { 'A', ':', '\\', '\0' };
    name[0] = drive;

    if (!GetVolumeNameForVolumeMountPoint(name, guid, sizeof(guid) - 1)) {
	    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return;
    }

    for (MONITORS::iterator iter = m_Monitors.begin(); iter != m_Monitors.end(); ++iter) {
        if ((*iter).second.Device() == guid) {
            if (!UnregisterDeviceNotification((*iter).first)) {
                DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::UnregisterDriveLetter UnregisterDeviceNotification for device %s failed: %d\n", (*iter).second.Device().c_str(), GetLastError());                              
            }
            m_Monitors.erase(iter);
            break;
        }
    } 

    //Unlock();    
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    
}

bool VolumeMonitor::RegisterDrives()
{    
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    bool ok = true;

    char volName[MAX_PATH];

    HANDLE h = FindFirstVolume(volName, sizeof(volName) -1);
    if (INVALID_HANDLE_VALUE == h) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::RegisterDrives FindFirstVolume failed: %d\n", GetLastError()); 
        return false;
    }

    boost::shared_ptr<void> guard((void*)NULL, boost::bind<BOOL>(FindVolumeClose, h));

    do {        
        if (DRIVE_FIXED == GetDriveType(volName)) {  
            // need to replace the '?' with '.' and remove the trailing '\' so RegisterDevice can open the volume
            volName[2] = '.';                  
            volName[strlen(volName)-1] = '\0'; 

            if (!RegisterDevice(volName)) {
                ok = false;
            }
        }       
    } while (FindNextVolume(h, volName, sizeof(volName) - 1));

	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return ok;
}
        
bool VolumeMonitor::RegisterDriveLetter(char drive)
{       
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    char guid[128];

    char name[4] = { 'A', ':', '\\', '\0' };
    name[0] = drive;

    if (!GetVolumeNameForVolumeMountPoint(name, guid, sizeof(guid) - 1)) {
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return false;
    }
   
    // need to replace the '?' with '.' and remove the trailing '\' so RegisterDevice can open the volume
    guid[2] = '.';
    guid[strlen(guid)-1] = '\0';

	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return RegisterDevice(guid);
}

bool VolumeMonitor::RegisterVolume(char const * volume)
{
 	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    char name[1024];

	inm_sprintf_s(name, ARRAYSIZE(name), "%s", volume);

	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return RegisterDevice(name);
}

bool VolumeMonitor::RegisterDevice(char const * name)
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    bool ok = true;

	// PR#10815: Long Path support
    HANDLE hFile = SVCreateFile(name,
                              0, //QUERY is good enough
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);            
    if (INVALID_HANDLE_VALUE == hFile) { 
        unsigned long rc = GetLastError();
        if (ERROR_UNRECOGNIZED_VOLUME != rc) {
            DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::RegisterDevice open file %s failed: %d\n", name, GetLastError());
        }
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return false;
    }

    DEV_BROADCAST_HANDLE handleFilter;

    ZeroMemory(&handleFilter, sizeof(handleFilter));
    handleFilter.dbch_size       =  sizeof(DEV_BROADCAST_HANDLE);
    handleFilter.dbch_devicetype = DBT_DEVTYP_HANDLE;
    handleFilter.dbch_handle     = hFile;
    HDEVNOTIFY notify = RegisterDeviceNotification(m_RegisterHandle, &handleFilter, m_RegisterHandleType);
    if (0 == notify) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::RegisterDevice RegisterDeviceNotification for %s failed: %d\n", name, GetLastError());
        ok = false;
    } else {
/*        if (!Lock()) {
            DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::RegisterDevice Lock failed: %d\n", GetLastError());
            ok = false;
        } else*/
        AutoLock LockSection(SyncLock);
//        {
            m_Monitors.insert(MONITOR_PAIR(notify, VolumeState(name, GUID_IO_VOLUME_MOUNT)));
            //Unlock();
//        }
    }
    CloseHandle(hFile); 
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);

    return ok;
}

bool VolumeMonitor::Process(WPARAM wparam, LPARAM lparam)
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    bool callRegisterHost = false;
    try {
        SIMPLE_TRACE("VolumeMonitor::Process\n");        
        switch (wparam) {
        case DBT_CUSTOMEVENT:  
            SIMPLE_TRACE("VolumeMonitor DBT_CUSTOMEVENT\n");
            if ( true == (callRegisterHost = ProcessCustomEvent((DEV_BROADCAST_HDR*)lparam)))
                SignalClientIfRequired();
            break;
        case DBT_DEVICEARRIVAL:                        
            SIMPLE_TRACE("VolumeMonitor DBT_DEVICEARRIVAL\n");
            if ( true == (callRegisterHost = ProcessArrival((DEV_BROADCAST_HDR*)lparam)))
                SignalClientIfRequired();
            break;
        case DBT_DEVICEQUERYREMOVEFAILED:                   
            SIMPLE_TRACE("VolumeMonitor DBT_DEVICEQUERYREMOVEFAILED\n");            
            break;
        case DBT_DEVICEREMOVECOMPLETE:                 
            SIMPLE_TRACE("VolumeMonitor DBT_DEVICEREMOVECOMPLETE\n");
            if ( true == (callRegisterHost = ProcessRemoveComplete((DEV_BROADCAST_HDR*)lparam)))
                SignalClientIfRequired();
            break;
        case DBT_DEVICEREMOVEPENDING:            
            SIMPLE_TRACE("VolumeMonitor DBT_DEVICEREMOVEPENDING\n");
            break;
        case DBT_DEVICETYPESPECIFIC:            
            SIMPLE_TRACE("VolumeMonitor DBT_DEVICETYPESPECIFIC\n");
            // TODO:
            // may need to process this, if so, it would be more or less the same as DBT_CUSTOMEVENT
            break;
        case DBT_DEVNODES_CHANGED:            
            SIMPLE_TRACE("VolumeMonitor DBT_DEVNODES_CHANGED\n");
            break;
        case DBT_USERDEFINED:            
            SIMPLE_TRACE("VolumeMonitor DBT_USERDEFINED\n");
            break;
        case DBT_CONFIGCHANGECANCELED:            
            SIMPLE_TRACE("VolumeMonitor DBT_CONFIGCHANGECANCELED\n");
            break;
        case DBT_CONFIGCHANGED:            
            SIMPLE_TRACE("VolumeMonitor DBT_CONFIGCHANGED\n");
            break;
        case DBT_DEVICEQUERYREMOVE:            
            SIMPLE_TRACE("VolumeMonitor DBT_DEVICEQUERYREMOVE\n");                       
            break;
        default:                   
            SIMPLE_TRACE("VolumeMonitor default\n");
            break;
        }
    } catch (std::exception const & e) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Process failed: %s\n", e.what());
        callRegisterHost = true; // play it safe force a registration
        SignalClientIfRequired();
    } catch (...) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Process failed with unknown exception\n");
        callRegisterHost = true; // play it safe force a registration
        SignalClientIfRequired();
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);

    return callRegisterHost;
}

bool VolumeMonitor::ProcessCustomEvent(DEV_BROADCAST_HDR* hdr)
{
    if (0 == hdr || DBT_DEVTYP_HANDLE != hdr->dbch_devicetype) {
        return false;
    }
 
/*    if (!Lock()) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::ProcessCustomEvent Lock failed: %d\n", GetLastError());
        return false;
    }
*/

    AutoLock LockSection(SyncLock);

    DEV_BROADCAST_HANDLE* notify = (DEV_BROADCAST_HANDLE*)hdr;

    SIMPLE_TRACE("VolumeMonitor::ProcessCustomEvent GUID:"
        << std::hex
        << notify->dbch_eventguid.Data1 << '-'
        << notify->dbch_eventguid.Data2 << '-'
        << notify->dbch_eventguid.Data3 << '-'
        << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[0] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[1] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[2] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[3]
        << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[4] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[5] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[6] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbch_eventguid.Data4[7]
        << '\n');

    bool callRegisterHost = false;

    MONITORS::iterator iter = m_Monitors.find(notify->dbch_hdevnotify);
    if (iter != m_Monitors.end()) {
        SIMPLE_TRACE("VolumeMonitor::ProcessCustomEvent current state:"
                    << std::hex
                    << (*iter).second.State().Data1 << '-'
                    << (*iter).second.State().Data2 << '-'
                    << (*iter).second.State().Data3 << '-'
                    << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[0] << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[1] << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[2] << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[3]
                    << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[4] << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[5] << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[6] << std::setfill('0') << std::setw(2) << (unsigned)(*iter).second.State().Data4[7]
                    << '\n');

        if ((IsEqualGUID(GUID_IO_VOLUME_NAME_CHANGE, notify->dbch_eventguid) && !IsEqualGUID(GUID_IO_VOLUME_NAME_CHANGE, (*iter).second.State())) ||
            (IsEqualGUID(GUID_IO_VOLUME_CHANGE, notify->dbch_eventguid)) ||
            (IsEqualGUID(GUID_IO_VOLUME_FVE_STATUS_CHANGE, notify->dbch_eventguid))) {

            std::string vol((*iter).second.Device());
            vol += "\\";

            int type = GetDriveType(vol.c_str());
            if (DRIVE_FIXED == type) {
                SIMPLE_TRACE("VolumeMonitor::ProcessCustomEvent need to register host: volume: " << vol[0] << '\n');
                callRegisterHost = true;

                if (IsEqualGUID(GUID_IO_VOLUME_FVE_STATUS_CHANGE, notify->dbch_eventguid))
                    SignalBitlockerEventToClient();
            }
        }
        (*iter).second.SetState(notify->dbch_eventguid);
    }
    //Unlock();
    return callRegisterHost;
}

bool VolumeMonitor::ProcessArrival(DEV_BROADCAST_HDR* hdr)
{    
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    if (0 == hdr) { 
        return false;
    }
    SIMPLE_TRACE("VolumeMonitor::ProcessArrival device type: " << hdr->dbch_devicetype << '\n');
    switch (hdr->dbch_devicetype ) {
        case DBT_DEVTYP_VOLUME: 
            {                
                DEV_BROADCAST_VOLUME* notify = (DEV_BROADCAST_VOLUME*)hdr;
                SIMPLE_TRACE("VolumeMonitor::ProcessArrival DBT_DEVTYP_VOLUME flag: " << notify->dbcv_flags << " unitmask: " << notify->dbcv_unitmask << '\n');
                if (DBTF_MEDIA != notify->dbcv_flags && DBTF_NET != notify->dbcv_flags) {
                    // for a DISK 
                    // get drive letter
                    for (int i = 0; i < 26; ++i) {
                        if (notify->dbcv_unitmask & (1 << i)) {
                            RegisterDriveLetter((char)(i + 'A'));
                            SIMPLE_TRACE("VolumeMonitor::ProcessArrival need to register host\n");
	                        DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
                            return true;
                        }
                    }
                }
            }
            break;
        case DBT_DEVTYP_DEVICEINTERFACE:
            {
                DEV_BROADCAST_DEVICEINTERFACE* notify = (DEV_BROADCAST_DEVICEINTERFACE*)hdr;
                SIMPLE_TRACE("VolumeMonitor::ProcessArrival DBT_DEVTYP_DEVICEINTERFACE class guid: " 
                    << std::hex 
                    << notify->dbcc_classguid.Data1 << '-'  
                    << notify->dbcc_classguid.Data2 << '-' 
                    << notify->dbcc_classguid.Data3 << '-' 
                    << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[0] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[1] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[2] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[3]
                    << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[4] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[5] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[6] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[7]
                    << '\n');
                if (GUID_DEVINTERFACE_VOLUME == notify->dbcc_classguid) { 
                    SIMPLE_TRACE("VolumeMonitor::ProcessArrival need to register host\n");
                    // since it is too much work to figure out which volume this is, just use brute force and redo everything.
                    ReregisterAll();
                    RegisterVolume(notify->dbcc_name);
                	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
                    return true;
                }
            }
            break;
        default:
            break;
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return false;
}

bool VolumeMonitor::ProcessRemoveComplete(DEV_BROADCAST_HDR* hdr)
{   
    if (0 == hdr) {
        return false;
    }

    SIMPLE_TRACE("VolumeMonitor::ProcessRemoveComplete device type: " << hdr->dbch_devicetype << '\n');

    switch (hdr->dbch_devicetype ) {
        case DBT_DEVTYP_VOLUME: 
            {
                DEV_BROADCAST_VOLUME* notify = (DEV_BROADCAST_VOLUME*)hdr;
                SIMPLE_TRACE("VolumeMonitor::ProcessRemoveComplete DBT_DEVTYP_VOLUME flag: " << notify->dbcv_flags << " unitmask: " << notify->dbcv_unitmask << '\n');
                if (DBTF_MEDIA != notify->dbcv_flags && DBTF_NET != notify->dbcv_flags) {
                    for (int i = 0; i < 26; ++i) {
                        if (notify->dbcv_unitmask & (1 << i)) {                        
                            UnregisterDriveLetter((char)(i + 'A'));
                            SIMPLE_TRACE("VolumeMonitor::ProcessRemoveComplete need to register host volume: " << (char)(i + 'A') << '\n');
                            return true;                
                        }
                    }
                }
            }
            break;
        case DBT_DEVTYP_DEVICEINTERFACE:
            {
                DEV_BROADCAST_DEVICEINTERFACE* notify = (DEV_BROADCAST_DEVICEINTERFACE*)hdr;
                SIMPLE_TRACE("VolumeMonitor::ProcessRemoveComplete DEV_BROADCAST_DEVICEINTERFACE class guid: " 
                    << std::hex
                    << notify->dbcc_classguid.Data1 << '-' 
                    << notify->dbcc_classguid.Data2 << '-' 
                    << notify->dbcc_classguid.Data3 << '-' 
                    << std::setfill('0') << std::setw(2) << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[0] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[1] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[2] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[3]
                    << std::setfill('0') << std::setw(2) << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[4] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[5] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[6] << std::setfill('0') << std::setw(2) << (unsigned)notify->dbcc_classguid.Data4[7]
                    << '\n');
                if (GUID_DEVINTERFACE_VOLUME == notify->dbcc_classguid) {  
                    SIMPLE_TRACE("VolumeMonitor::ProcessRemoveComplete need to register host\n");
                    // since it is too much work to figure out which volume this is, just use brute force and redo everything.
                    ReregisterAll();
                    return true;
                }
            }
            break;
        default:
            break;
    }

    return false;
}

bool VolumeMonitor::DriveOnline(char c)
{
   	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    bool ok = false;
/*    if (!Lock()) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::DriveOnline Lock failed: %d\n", GetLastError());
    } else 
*/    
    AutoLock LockSection(SyncLock);
    {        
        for (MONITORS::iterator iter = m_Monitors.begin();
            iter != m_Monitors.end();
            ++iter) 
        {
            if (1 == (*iter).second.Device().size() && c == (*iter).second.Device()[0]) {
                if (IsEqualGUID(GUID_IO_VOLUME_MOUNT, (*iter).second.State())) {                    
                    ok = true;
                    break;
                }
            }
        }        
        //Unlock();        
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return ok;
}

DWORD VolumeMonitor::CreateNotificationWindow()
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    
    DWORD result = ERROR_SUCCESS;
    
    char* className = "VolumeNotificationWindow";

    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(wc));

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = VolumeMonitor::NotifyWindowProc; // DefWindowProc; 
    wc.lpszClassName = className;

    m_Atom = RegisterClassEx(&wc);
    if (0 == m_Atom)  {
        result = GetLastError();        
    } else {

        m_Window = CreateWindow(className, "VolumeNotificationWindow", WS_OVERLAPPEDWINDOW, 0, 0, 300, 300, 0, 0, 0, 0);
        if (0 == m_Window) {
            result = GetLastError();            
        } else {
            SetLastError(0);
            if (!SetWindowLongPtr(m_Window, GWLP_USERDATA, (LONG_PTR)this)) {
                result = GetLastError();
            }
        }
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);

    return result;
}

bool VolumeMonitor::Start(HANDLE serviceHandle, DWORD type)
{   
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
 
    bool ok = false;
    try {
        if (m_Registered) {
            ok = true;
        } else {        
            ok = Register(serviceHandle, type);
        }
    } catch (std::exception const & e) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Start failed: %d\n", e.what());
        ok = false;
    } catch (...) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Start failed with unknown exception\n");
        ok = false;
    }

	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return ok;
}

bool VolumeMonitor::Start()
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    bool ok = true;

    if (0 == m_ThreadId) {
        m_Thread = (HANDLE)_beginthreadex(0, 0, VolumeMonitor::MonitorThread, (void*)this, 0, &m_ThreadId);
        // m_Thread = CreateThread(NULL, 0, VolumeMonitor::MonitorThread, this, 0, &m_ThreadId);
        if (0 == m_Thread) {
            ok = false;            
            DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::Start Error creating cluster monitor thread: %d\n", GetLastError());
        } else {
            // wait for registration to complete
            ok = Wait();
        }
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return ok;

}

void VolumeMonitor::Stop()
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    UnregisterAll();
    m_Monitors.clear();

    if (0 != m_ThreadId) {
        PostThreadMessage(m_ThreadId, WM_QUIT, 0, 0);
        WaitForSingleObject(m_Thread, INFINITE);
        m_ThreadId = 0;
        CloseHandle(m_Thread);
        m_Thread = 0;
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
}

unsigned __stdcall VolumeMonitor::MonitorThread(LPVOID pParam)
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    try {
        if (0 != pParam) {
            VolumeMonitor* volumeMonitor = (VolumeMonitor*) pParam;

            volumeMonitor->Register();        

            MSG msg;
            BOOL ret;
            do {
                ret = GetMessage(&msg, (HWND) NULL, 0, 0);
                if (ret <= 0) {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg); 
            } while (1);
        }    
    } catch (std::exception const & e) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::MonitorThread failed: %d\n", e.what());
	    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return -1;
    } catch (...) {
        DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::MonitorThread failed with unknown exception\n");
    	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
        return -1;
    }
	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return 0;
}

LRESULT CALLBACK VolumeMonitor::NotifyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    LRESULT result = 0;
    if (WM_DEVICECHANGE == uMsg) {
        VolumeMonitor* volumeMonitor = (VolumeMonitor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (0 == volumeMonitor) {
            DebugPrintf(SV_LOG_ERROR, "VolumeMonitor::NotifyWindowProc error geting volumemonitor: %d\n", GetLastError());
        } else {
            volumeMonitor->Process(wParam, lParam);
        }
    } else {
        result = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

	DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return result;
}

std::string VolumeMonitor::GetVolumesOnlineAsString()
{
    std::string  sVolumes = "";

/*    if (!Lock()) {
        DebugPrintf(SV_LOG_ERROR, "Lock failed: %d\n", GetLastError());
    } else 
*/    
    AutoLock LockSection(SyncLock);
    {
        for (MONITORS::iterator iter = m_Monitors.begin();
            iter != m_Monitors.end();
            ++iter) {                
                if (IsEqualGUID(GUID_IO_VOLUME_MOUNT, (*iter).second.State())
                    && (*iter).second.Device() != "!" ) { // don't want the interface monitor it's tag is '!'
                    sVolumes += ";" + ((*iter).second.Device());
                }
            }
            //Unlock();
    }    
    return sVolumes;
}

sigslot::signal1<void*>& VolumeMonitor::getVolumeMonitorSignal() { 
    return m_VolumeMonitorSignal;
}

void VolumeMonitor::SignalClientIfRequired()
{
    if ( m_ClientSignal )
        // Pass NULL for now. Maybe we want to pass the list of modified volumes
        // or volume list
        m_VolumeMonitorSignal(NULL);
}

sigslot::signal1<void*>& VolumeMonitor::getBitlockerMonitorSignal() {
    return m_BitlockerMonitorSignal;
}

void VolumeMonitor::SignalBitlockerEventToClient()
{
    if (m_ClientSignal)
        m_BitlockerMonitorSignal(NULL);
}