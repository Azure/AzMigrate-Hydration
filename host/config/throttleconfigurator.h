#ifndef THROTTLE_CONFIGURATOR__H
#define THROTTLE_CONFIGURATOR__H

#include <string>
#include <map>
//#include <windows.h>
#include "svtypes.h"
#include "s2libsthread.h"

typedef enum 
{
    THRESHOLD_RESYNC = 0,
    THRESHOLD_SENTINEL,
    THRESHOLD_OUTPOST
}THRESHOLD_AGENT;

typedef enum
{
    THROTTLE_PARSER_OK,        
    THROTTLE_PARSER_INVALID_STRING,
    THROTTLE_PARSER_INFO_UNAVAILABLE
}THROTTLE_PARSER_STATUS;

const char THROTTLE_SET_DELIMITER = ';';
const char THROTTLE_SUB_DELIMITER = ',';

typedef  unsigned int UINT32;
const UINT32 DEFAULT_POLL_INTERAVAL=(1*60);
const UINT32 DEFAULT_CLIENT_WAIT_TIME=(1*60);


class Event;
class Thread;
class Lockable;

class ThrottleStringParser
{
public:
    ThrottleStringParser(std::string throttleString = "");
    virtual ~ThrottleStringParser();
    void SetString(const std::string& throttleString);
    const std::string& GetString()const;
    THROTTLE_PARSER_STATUS GetThrottleInfo(const std::string deviceName,THRESHOLD_AGENT thresholdAgent,bool* throttleInfo)const;

protected:
    void ParseString();
    THROTTLE_PARSER_STATUS ParseSubString(std::string& subString);
private:
    std::string parseString; 
    THROTTLE_PARSER_STATUS ParserStatus;
    typedef struct 
    {
        bool  ResyncThreshold;
        bool  SentinelThreshold;
        bool OutpostThreshold;
    }ThrottleInfo;
    typedef std::map<std::string,ThrottleInfo> ThrottleDeviceMap;
    ThrottleDeviceMap throttleMap;
    Lockable *ParserLock;
};


class ThrottleConfigurator
{
    public:
        ThrottleConfigurator(UINT32 pollInterval = DEFAULT_POLL_INTERAVAL);                
        virtual ~ThrottleConfigurator();

    private:
        //Note: This class was originally designed as singleton.
        //so to make it singleton, move this function to public 
        //and constructor to private
        static ThrottleConfigurator*  GetInstance(UINT32 pollInterval = DEFAULT_POLL_INTERAVAL);
       


    public:
        SVERROR CheckThrottle(std::string deviceName,THRESHOLD_AGENT thresholdAgent, bool *isThrottled);

    protected:
        void PollThrottleInfo();
        SVERROR GetThrottleInfo();

    private:
#ifdef SV_WINDOWS /* TODO: This code will be re-arranged soon */
        static THREAD_FUNC_RETURN ThreadProc( void * context ); 
#endif
        void Stop();

    private:
        UINT32 m_pollInterval;
        bool m_gotThrottleInfo;
        ThrottleStringParser throttleString;
   	    Thread* m_hThread;
	    Event* m_hQuit;
        Event* m_hGotInfo;
        static Lockable* m_InstanceCreate;
        //Note: pointers to object are used to avoid circular dependencies :  once this issue is resolved 
        //we will have to get back to object instad of pointers;
};
#endif

