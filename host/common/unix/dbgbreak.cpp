//                                       
// Copyright (c) 2005 InMage.Debug
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use,
// disclosure, or reproduction is prohibited except as permitted
// by express written license agreement with InMage.
// 
// File       : dbgbreak.cpp
// 
// Description: a simple way to suspend a process and then manually attach gdb
//              or automactially launch gdb.
//
//              once gdb is attached you need to issue a signal to unsuspend the process
//              issue signal SIGUSR1 if you want to immediately break 
//              issue signal SIGUSR2 to continue with out breaking
//
//              NOTES:
//              1. if you do not attach a debugger and issue SIGUSR1, the process will
//                 most likely report a Trace/breakpoint trap and exit (as the BREAK_INSTRUCTION
//                 will not be handled)
//              2. issuing any other signal may also cause the process to continue without breaking
//                 in the debugger or even cause the process to exit (even if a debugger is attached
//                 if there is no handler for it).
//

#include <sys/types.h>
#include <signal.h>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <pthread.h>
#include <fstream>
#include <sstream>
#include <string>

#include "dbgbreak.h"
#include "inmsafecapis.h"

pthread_mutex_t g_AttachDebuggerIfNeededLock = PTHREAD_MUTEX_INITIALIZER;

bool g_DebuggerAttached = false;
bool g_TrapHandled = false;
bool g_Break = false;
bool g_SkipBreak = false;

void SIGUSR1DbgBreakHandler(int)
{
    // break in the debugger
    // not if no debugger attached this may cause the program to exit
    // as there will be no handler for the BREAK_INSTRUCTION
    g_Break = true;
}

void SIGUSR2DbgBreakHandler(int)
{
    // empty signal handler needed to get sigsuspend to return
    // with out breaking in the debugger
}

void SIGTRAPDbgBreakHandler(int)
{
    // if we handle this trap then not running under debugger
    // this is to detect manual gdb attach so that
    // we don't attempt to launch gdb automatically
    g_TrapHandled = true;
}

bool DebuggerManuallyAttached()
{
    struct sigaction SIGTRAPHandler;
    struct sigaction oldSIGTRAPHandler;
    
    memset(&SIGTRAPHandler, 0, sizeof(SIGTRAPHandler));
    SIGTRAPHandler.sa_handler = SIGTRAPDbgBreakHandler;    
    sigaction(SIGTRAP, &SIGTRAPHandler, &oldSIGTRAPHandler);
    BREAK_INSTRUCTION;
    sigaction(SIGTRAP, &oldSIGTRAPHandler, NULL);

    if (g_TrapHandled) {
        g_TrapHandled = false; // reset for next time
    } else {
        g_DebuggerAttached = true;
    }

    return g_DebuggerAttached;
}

bool AttachDebuggerIfNeeded(bool launchDebugger)
{
    bool suspend = false;  // assume we don't want to suspend
    
    // not 100% thread safe but should be ok
    // (search the web about double check lock issues)
    if (!g_DebuggerAttached) {
        if (0 == pthread_mutex_lock(&g_AttachDebuggerIfNeededLock)) {
            if (!g_DebuggerAttached) {
                if (!DebuggerManuallyAttached()) {
                    if (!launchDebugger) {
                        suspend = true; // not running under debugger and not launching need to suspend
                    } else {
                        try {
                            std::ostringstream exeCmdlineName;
                            exeCmdlineName << "/proc/" << getpid() << "/cmdline";
                            
                            std::ifstream exeCmdlineFile(exeCmdlineName.str().c_str());
                            
                            if (exeCmdlineFile.good()) {
                                std::string exeName;
                                
                                exeCmdlineFile >> exeName;
                                
                                char exeNameBuf[256]; // should be some type of MAXPATH
                                
								inm_sprintf_s(exeNameBuf, ARRAYSIZE(exeNameBuf), "%s", exeName.c_str());
                                
                                char pid[32];
                                
								inm_sprintf_s(pid, ARRAYSIZE(pid), "--pid=%d", getpid());
                                
                                char gdb[4] = {'g', 'd', 'b', '\0' };
                                
                                char* args[4];
                                args[0] = gdb;
                                args[1] = pid;
                                args[2]= exeNameBuf;
                                args[3]= (char*)0;        
                                
                                if (0 == fork()) {
                                    execvp(args[0], args);
                                    _exit(0); // only get here if execvp had an error so just exit child
                                }
                                
                                g_DebuggerAttached = true;
                                suspend = true; // first time entering debugger so we need to suspend
                            }
                        } catch(...) {
                            suspend = true; // had problem attaching debugger so suspend
                        }
                    }
                }
            }
            pthread_mutex_unlock(&g_AttachDebuggerIfNeededLock);
        } else {
            suspend = true; // problem with lock, couldn't attach debugger so suspend
        }
    }
    
    if (g_DebuggerAttached) {
        g_Break = true; // always break once running under the debugger
    }
    
    return suspend;
}

void DbgBreak(bool launchDebugger)
{
    struct sigaction SIGUSR1Handler;
    struct sigaction SIGUSR2Handler;
    struct sigaction oldSIGUSR1Handler;
    struct sigaction oldSIGUSR2Handler;

    memset(&SIGUSR1Handler, 0, sizeof(SIGUSR1Handler));
    SIGUSR1Handler.sa_handler = SIGUSR1DbgBreakHandler;    
    sigaction(SIGUSR1, &SIGUSR1Handler, &oldSIGUSR1Handler);

    memset(&SIGUSR2Handler, 0, sizeof(SIGUSR2Handler));
    SIGUSR2Handler.sa_handler = SIGUSR2DbgBreakHandler;    
    sigaction(SIGUSR2, &SIGUSR2Handler, &oldSIGUSR2Handler);

    sigset_t sigset;
    sigemptyset(&sigset);

    if (AttachDebuggerIfNeeded(launchDebugger)) {
        sigsuspend(&sigset);
    }
    
    sigaction(SIGUSR1, &oldSIGUSR1Handler, NULL);
    sigaction(SIGUSR2, &oldSIGUSR2Handler, NULL);
    
    if (g_Break && !g_SkipBreak) {
        g_Break = false;
        // NOTE: after sending sginal SIGUSR1 the debugger will break at this point.
        // although the debugger may actually place you at the last closing '}'
        BREAK_INSTRUCTION; 
    }

}
