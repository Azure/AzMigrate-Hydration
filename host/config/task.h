#ifndef TASK_H
#define TASK_H
#include "portablehelpers.h"
enum InmTaskState
{
  //  TASK_STATE_DISABLED,
    TASK_STATE_NOT_STARTED,
    TASK_STATE_SCHEDULED,
    TASK_STATE_STARTED,
    TASK_STATE_RUNNING,
    TASK_STATE_COMPLETED,
    TASK_STATE_FAILED,
    TASK_STATE_DELETED
} ;

struct TaskInformation
{
    SV_ULONG m_RunEvery;
	SV_ULONGLONG m_StartTime;
	SV_ULONGLONG m_firstTriggerTime;
	SV_ULONGLONG m_LastExecutionTime;
	SV_ULONGLONG m_NextExecutionTime;
	SV_ULONGLONG m_ScheduleExcludeFromTime;
	SV_ULONGLONG m_ScheduleExcludeToTime;
	InmTaskState m_State;
    std::string m_ErrorMessage;
    std::string hostId;
    bool m_bIsActive;
    SV_ULONGLONG m_InstanceId;
    SV_ULONGLONG m_PreviousScheduleTickCount;
    SV_ULONGLONG m_NextScheduleTickCount;
    bool m_bRemoveFromExisting;
public:
    TaskInformation()
    {
        m_RunEvery = 0 ;
        m_StartTime = 0 ;
        m_LastExecutionTime = 0 ;
        m_NextExecutionTime = 0 ;
        m_ScheduleExcludeFromTime = 0 ;
        m_ScheduleExcludeToTime = 0 ;
        m_State = TASK_STATE_NOT_STARTED ;
        m_bIsActive = true;
        m_firstTriggerTime = 0;
        m_PreviousScheduleTickCount = 0;
        m_NextScheduleTickCount = 0;
        m_bRemoveFromExisting = false;
    }
    
    TaskInformation(SV_ULONG frequency)
    {
    
        m_RunEvery          = frequency ;
        m_StartTime         = 0 ;
        m_LastExecutionTime = 0 ;
        m_NextExecutionTime = 0 ;
        m_State         = TASK_STATE_NOT_STARTED ;
        m_ScheduleExcludeFromTime = 0 ;
        m_ScheduleExcludeToTime = 0 ;
        m_bIsActive =  true;
        m_firstTriggerTime = 0;
        m_PreviousScheduleTickCount = 0;
        m_NextScheduleTickCount = 0;
        m_bRemoveFromExisting = false;
    }

};


struct AppSchedulingInfo
{
    std::map<SV_ULONG, TaskInformation> m_activeTasks ; 
} ;

#endif
