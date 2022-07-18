#ifndef _H__INM__DEFINES
#define _H__INM__DEFINES

#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <cstdlib>
#include "svtypes.h"

const char * const StrTriState[] = {"unknown", "false", "true"};
#define STRTRISTATE(x) StrTriState[(int)(x)+1]

typedef enum InmTriState
{
    E_INM_TRISTATE_FLOATING = -1,
    E_INM_TRISTATE_FALSE,
    E_INM_TRISTATE_TRUE
    
} E_INM_TRISTATE;

typedef enum InmAttrState
{    
E_INM_ATTR_STATE_DISABLED,
E_INM_ATTR_STATE_ENABLED,
E_INM_ATTR_STATE_ENABLEIFAVAILABLE
}InmAttrState_t;

typedef InmAttrState_t InmSparseAttributeState_t;

typedef enum eEndianness 
{ 
    ENDIANNESS_UNKNOWN = 0, 
    ENDIANNESS_LITTLE = 1, 
    ENDIANNESS_BIG = 2, 
    ENDIANNESS_MIDDLE = 3 
} ENDIANNESS;


#define INMLOCKEXT ".lck"

typedef enum eState
{
    STATE_NORMAL,
    STATE_EXCEPTION

} State_t;

const char * const StrState[] = {"normal", "exception"};

typedef struct stStatus
{
    State_t m_State;
    std::string m_ExcMsg;

    stStatus() : 
    m_State(STATE_NORMAL)
    {
    }

    stStatus(const State_t st, const std::string &excmsg)
    {
        m_State = st;
        m_ExcMsg = excmsg;
    }
       
} Status_t;
typedef std::vector<Status_t> Statuses_t;
typedef Statuses_t::const_iterator ConstStatusesIter_t;
typedef Statuses_t::iterator StatusesIter_t;

const char * const StrFeatureSupportState[] = {"feature supported", "feature not supported", "feature cant say"};
typedef enum eFeatureSupportState
{
    E_FEATURESUPPORTED,
    E_FEATURENOTSUPPORTED,
    E_FEATURECANTSAY

} FeatureSupportState_t;


struct DataAndLength
{
    char *m_pData;
    unsigned int m_length;

    DataAndLength():
    m_pData(0),
    m_length(0)
    {
    }

    void Print(void);
};


class DataWithUsedLength
{
public:
    DataWithUsedLength():
     m_pData(0),
     m_Length(0),
     m_UsedLength(0)
    {
    }

    ~DataWithUsedLength()
    {
        Release();
    }

    void ResetUsedLength(void)
    {
        m_UsedLength = 0;
    }

    char *GetData(void)
    {
        return m_pData;
    }

    char *GetDataForUse(void)
    {
        return (m_pData+m_UsedLength);
    }

    bool IsSpaceAvailable(const SV_UINT &additionallength)
    {
        return ((m_UsedLength+additionallength) <= m_Length);
    }

    void AddToUsedLength(const SV_UINT &additionallength)
    {
        m_UsedLength += additionallength;
    }

	SV_UINT GetLength(void)
	{
		return m_Length;
	}

    SV_UINT GetUsedLength(void)
    {
        return m_UsedLength;
    }

    void SetUsedLength(const SV_UINT &length)
    {
        m_UsedLength = length;
    }

    bool Allocate(const SV_UINT &length)
    {
        Release();

        m_pData = (char *) malloc(length);
        if (m_pData)
        {
            m_Length = length;
        }

        return m_pData ? true : false;
    }

    void Release(void)
    {
        if (m_pData)
        {
            free(m_pData);
            m_pData = 0;
        }
        m_Length = 0;
        m_UsedLength = 0;
    }

private:
    char *m_pData;
    SV_UINT m_Length;
    SV_UINT m_UsedLength;
};


typedef struct stCapacity
{
    SV_ULONGLONG m_Nblks;
    SV_ULONGLONG m_BlkSz;

    stCapacity()
    {
        m_Nblks = 0;
        m_BlkSz = 0;
    }

} Capacity_t;


template <class INT>
void InmHexStrToInteger(const std::string &s, INT &i)
{
    std::stringstream ss(s);

    ss >> std::hex >> i;
}


#endif /* _H__INM__DEFINES */
