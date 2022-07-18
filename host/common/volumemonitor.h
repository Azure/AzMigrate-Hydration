//                                       
// Copyright (c) 2005 InMage.
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use,
// disclosure, or reproduction is prohibited except as permitted
// by express written license agreement with InMage.
// 
// File       : volumemonitor.h
// 
// Description: 
// 

#ifndef VOLUMEMONITOR__H
#define VOLUMEMONITOR__H

#include <windows.h>
#include <dbt.h>
#include <map>
#include "sigslot.h"
#include "synchronize.h"

// FIXME:
// need to add support for volumes that don't have don't have drive letters

class VolumeMonitor {
public:
    VolumeMonitor() 
        : m_Registered(false), 
          m_RegisterHandle(0), 
          m_RegisterHandleType(0),
          m_Window(0),
          m_Atom(0),
          m_ThreadId(0),
          m_Thread(0),
          m_ClientSignal(false) { }

    ~VolumeMonitor();

    bool Start();
    bool Start(HANDLE h, DWORD type);    
        
    bool DriveOnline(char drive);    

    bool Process(WPARAM wparam, LPARAM lparam);

    bool Wait() { return m_Event.Wait(); }

    void Stop();

    std::string GetVolumesOnlineAsString();
       
    sigslot::signal1<void*>& getVolumeMonitorSignal();

    sigslot::signal1<void*>& getBitlockerMonitorSignal();

protected:

    static unsigned __stdcall MonitorThread(LPVOID pParam);

    static LRESULT CALLBACK NotifyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
      
    void Signal() { m_Event.Set(); }    

    class VolumeState {
    public:
        VolumeState(std::string const & name, GUID g) : m_Device(name), m_State(g) { }
        void SetState(GUID g) { m_State = g; }
        std::string Device() { return m_Device; }
        GUID State() { return m_State; }
    protected:

    private:
        std::string m_Device; // drive letter
        GUID m_State; // set to either GUID_IO_VOLUME_MOUNT or GUID_IO_VOLUME_DISMOUNT

    };

    void UnregisterDriveLetter(char drive);

    bool Register();
    bool Register(HANDLE h, DWORD handleType);    
    bool ReregisterAll();
    bool UnregisterAll();
    bool RegisterDrives();
    bool RegisterDriveLetter(char drive);
    bool RegisterVolume(char const * volume);
    bool RegisterDevice(char const * volume);
    bool ProcessCustomEvent(DEV_BROADCAST_HDR* hdr);
    bool ProcessArrival(DEV_BROADCAST_HDR* hdr);
    bool ProcessRemoveComplete(DEV_BROADCAST_HDR* hdr);        

    DWORD CreateNotificationWindow();
    
private:
    typedef std::map<HDEVNOTIFY, VolumeState>  MONITORS;
    typedef std::pair<HDEVNOTIFY, VolumeState> MONITOR_PAIR;

    sigslot::signal1<void*> m_VolumeMonitorSignal;
    bool m_ClientSignal;
    void SignalClientIfRequired();

    sigslot::signal1<void*> m_BitlockerMonitorSignal;
    void SignalBitlockerEventToClient();

    VolumeMonitor(VolumeMonitor const &);
    VolumeMonitor& operator=(VolumeMonitor const &);

    bool m_Registered;
    
    MONITORS m_Monitors;

    HANDLE m_RegisterHandle;
    DWORD m_RegisterHandleType;
    
    HWND m_Window; 
    ATOM m_Atom;   

    unsigned m_ThreadId;
    HANDLE m_Thread;

    // FIXME:
    // switch to a common lock once available
/*    class SimpleLock {
    public:
        SimpleLock();
        ~SimpleLock();
        bool Lock();
        void Unlock();
    protected:
    private:
        HANDLE m_Lock;
    } m_Lock;    
    bool Lock()   { return m_Lock.Lock(); }
    void Unlock() { return m_Lock.Unlock(); }
*/
    static Lockable SyncLock;

    // FIXME:
    // switch to a common event once available
    class SimpleEvent {
    public:
        SimpleEvent();
        ~SimpleEvent();
        bool Wait();
        void Set();
    protected:
    private:
        HANDLE m_Event;
    } m_Event;    
};

#endif // ifndef VOLUMEMONITOR__H
