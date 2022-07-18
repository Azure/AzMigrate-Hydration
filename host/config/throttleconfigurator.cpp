
#include <string>
#include <cassert>
#include "hostagenthelpers.h"
#include "event.h"
#include "s2libsthread.h"
#include "synchronize.h"
#include "throttleconfigurator.h"

Lockable * ThrottleConfigurator::m_InstanceCreate;

ThrottleConfigurator* ThrottleConfigurator ::  GetInstance(UINT32 pollInterval)
{
    m_InstanceCreate = new Lockable;
    AutoLock GetInstanceLock(*m_InstanceCreate);
    static ThrottleConfigurator ThrottleConfInstance(pollInterval);
    return &ThrottleConfInstance;
	
}
ThrottleConfigurator :: ThrottleConfigurator(UINT32 pollInterval)
:   m_pollInterval(pollInterval),
    m_gotThrottleInfo(false),
    m_hGotInfo(new Event(false,true)),
    m_hQuit(new Event(false,true)),
    m_hThread(new Thread)

{ 
    m_hThread->Start(ThreadProc,this);
} 


ThrottleConfigurator :: ~ThrottleConfigurator()
{
     Stop();
     delete m_hGotInfo;
     delete m_hQuit;
     delete m_hThread;
     delete m_InstanceCreate;
}

void ThrottleConfigurator :: Stop()
{
    m_hQuit->Signal(true);
    m_hThread->Stop();    
}



THREAD_FUNC_RETURN ThrottleConfigurator::ThreadProc( void * context )
{
	assert( NULL != context );
	ThrottleConfigurator* configurator = static_cast<ThrottleConfigurator*>( context );
	configurator->PollThrottleInfo();
    return( 0 );
}


void ThrottleConfigurator :: PollThrottleInfo()
{
    bool quitting = false;
    
    while( !quitting )
    {
        do
        {
            SVERROR rc;
            rc = GetThrottleInfo();
            if(rc.failed())
            {                
                DebugPrintf( SV_LOG_ERROR,"FAILED Couldn't poll Throttle information,\n" );
                break;
            }
            m_gotThrottleInfo = true;
            if(!m_hGotInfo->IsSignalled())
            {
                m_hGotInfo->Signal(true);
            }            
        }while(false);
        // quit unless Wait return time out
        quitting = !(WAIT_TIMED_OUT == m_hQuit->Wait(m_pollInterval));
	}
}        

SVERROR ThrottleConfigurator :: GetThrottleInfo()
{
    SVERROR rc;

	const char* SV_ROOT_KEY = "SOFTWARE\\SV Systems";
    SV_HOST_AGENT_PARAMS sv_host_agent_params = {0};
    HRESULT hr = NULL;
    hr = GetAgentParamsFromRegistry( SV_ROOT_KEY, &sv_host_agent_params );
    if( FAILED( hr ) )
    {
        DebugPrintf( SV_LOG_ERROR,"@ LINE %d in FILE %s \n", LINE_NO, FILE_NAME );
        DebugPrintf( hr, "FAILED GetAgentParamsFromRegistry()... hr = %08X\n", hr );
        rc = SVE_FAIL;
    }    
	else
	{
        char *pszGetBuffer = NULL;
        std::string hostId = sv_host_agent_params.szSVHostID;
        std::string szThrottleURL = sv_host_agent_params.szThrottleURL;
        szThrottleURL += "?hostid=";
        szThrottleURL += hostId;

        rc = GetFromSVServer( sv_host_agent_params.szSVServerName,sv_host_agent_params.ServerHttpPort,szThrottleURL.c_str(), &pszGetBuffer );
        if ( rc.succeeded() )
		{
            if ( throttleString.GetString() != pszGetBuffer )
			{
                DebugPrintf( SV_LOG_INFO,"Throttling string changed: %s",pszGetBuffer);
                throttleString.SetString(pszGetBuffer);
			}
		}
        else
        {
            
            throttleString.SetString(""); //Throttle all the pairs
            DebugPrintf( SV_LOG_ERROR,"@ LINE %d in FILE %s \n", LINE_NO, FILE_NAME );
            DebugPrintf( SV_LOG_ERROR,"Problem in GetFromSVServer: %s",pszGetBuffer);
            DebugPrintf( SV_LOG_ERROR,"FAILED GetFromSVServer()... hr = %08X\n", rc.error.hr);
        }

        if(pszGetBuffer) delete[] pszGetBuffer;
    }
    return rc;
}

SVERROR  ThrottleConfigurator :: CheckThrottle(std::string deviceName,THRESHOLD_AGENT thresholdAgent, bool* isThrottled)	    
{
    SVERROR rc;
    *isThrottled = true;
    do
    {
        if(!m_gotThrottleInfo)
        {
            WAIT_STATE dwWait = m_hGotInfo->Wait(DEFAULT_CLIENT_WAIT_TIME); 
            bool isReady = dwWait == WAIT_SUCCESS;
            if(!isReady)
            {
                rc = SVE_FAIL;
                break;
            }
        } 
        if(throttleString.GetThrottleInfo(deviceName,thresholdAgent,isThrottled)== THROTTLE_PARSER_INVALID_STRING)       
        {
            DebugPrintf( SV_LOG_DEBUG,"Parsing error");
             rc = SVE_FAIL;
             break;           
        } 
    }while(false);
    return (rc);
}

ThrottleStringParser :: ThrottleStringParser(std::string String)
:parseString(String),
ParserStatus(THROTTLE_PARSER_OK),
ParserLock(new Lockable)
{
    ParseString();
}

ThrottleStringParser :: ~ThrottleStringParser()
{
    delete ParserLock;
}

void ThrottleStringParser ::SetString(const std::string &throttleString)
{
    AutoLock Setparserlock(*ParserLock);
    if(parseString != throttleString)
    {
		parseString.clear();
        parseString = throttleString;
        ParseString();
    }
}

const std::string& ThrottleStringParser ::GetString()const
{
    return parseString;
}

THROTTLE_PARSER_STATUS ThrottleStringParser :: GetThrottleInfo(const std::string deviceName,THRESHOLD_AGENT thresholdAgent,bool* isThrottled)const
{
    AutoLock Setparserlock(*ParserLock);
    THROTTLE_PARSER_STATUS ReturnStatus = THROTTLE_PARSER_OK ;
    *isThrottled = true;

    if(ParserStatus == THROTTLE_PARSER_OK)
    {
        ThrottleDeviceMap ::const_iterator iter = throttleMap.find(deviceName);
        if(iter != throttleMap.end())
        {
            ThrottleInfo throttleInfo = iter->second;
            switch(thresholdAgent)
            {
                case THRESHOLD_RESYNC:
                {
                    *isThrottled = throttleInfo.ResyncThreshold;
                    break;
                }
                case THRESHOLD_SENTINEL:
                {
                    *isThrottled = throttleInfo.SentinelThreshold;
                    break;
                }
                case THRESHOLD_OUTPOST:
                {
                    *isThrottled = throttleInfo.OutpostThreshold;
                    break;
                }
                default:
                {
                    DebugPrintf(SV_LOG_FATAL,"Problem: agent couldn't be identified for device name: %s",deviceName.c_str());                    
                }
            }
        }       
        else 
        {
            DebugPrintf(SV_LOG_WARNING,"Throttle information is not available for this device name: %s",deviceName.c_str());
            ReturnStatus = THROTTLE_PARSER_INFO_UNAVAILABLE;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_WARNING,"Throttle string %s is invalid",parseString.c_str());
        ReturnStatus =  ParserStatus;
    }
    return ReturnStatus;
}

void ThrottleStringParser :: ParseString()
{
    throttleMap.clear();
    ParserStatus = THROTTLE_PARSER_OK; 
    if((!parseString.empty())&&(parseString != "\n"))
    {
        std::string::size_type position = 0;
        do
        {
            std::string ::size_type Pos;
            Pos = parseString.find(THROTTLE_SET_DELIMITER,position);           
            if(ParseSubString(parseString.substr(position,Pos-position))!= THROTTLE_PARSER_OK)
            {
                 //Dont process remaining string, set error status
                ParserStatus = THROTTLE_PARSER_INVALID_STRING;
                DebugPrintf(SV_LOG_WARNING,"Throttle string %s is invalid",parseString.c_str());
                throttleMap.clear();               
                break;
            }            
            position = Pos+1;            
            if(parseString[position] == '\n'||parseString[position] == '\0')
            {
                break;
            }
        } while(true);
    }
}

THROTTLE_PARSER_STATUS ThrottleStringParser :: ParseSubString(std::string& subString)
{
    THROTTLE_PARSER_STATUS returnStatus = THROTTLE_PARSER_OK;
    std::string ::size_type Pos = subString.find(THROTTLE_SUB_DELIMITER,0);
    if(Pos == std::string :: npos)
    {
        returnStatus = THROTTLE_PARSER_INVALID_STRING;
    }
    else
    {
        std::string deviceName = subString.substr(0,Pos);
        unsigned int index = 0;
        ThrottleInfo throttleInfo = {0};
        do
        {
            if(subString[Pos] != THROTTLE_SUB_DELIMITER)
            {
                //error : set error status
                returnStatus = THROTTLE_PARSER_INVALID_STRING;
                break;
            }
            Pos++;
            if(subString[Pos]!='0' && subString[Pos]!= '1')
            {
                //error : set error status
                returnStatus = THROTTLE_PARSER_INVALID_STRING;
                break;
            }
            if(index == 0) throttleInfo.ResyncThreshold = (subString[Pos] - '0') && 1; 
            else if(index == 1) throttleInfo.SentinelThreshold = (subString[Pos] - '0') && 1; 
            else if(index == 2)throttleInfo.OutpostThreshold = (subString[Pos] - '0') && 1; 

            index++;Pos++;

        }while(index < 3);

        if(Pos != subString.length())
        {
            //error : set error status
                returnStatus = THROTTLE_PARSER_INVALID_STRING;                
        }
        if(returnStatus != THROTTLE_PARSER_INVALID_STRING)
        {
            std::pair<std::string,ThrottleInfo> Throttlepair;
            ThrottleDeviceMap ::iterator iter = throttleMap.find(deviceName);
            if(iter != throttleMap.end())
            {
                iter->second = throttleInfo;
            }
            else
            {
                Throttlepair.first = deviceName;
                Throttlepair.second = throttleInfo;
                throttleMap.insert(Throttlepair);
            }
        }
    }
    return returnStatus;
}

