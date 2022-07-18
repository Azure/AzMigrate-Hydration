//
// Copyright (c) 2005 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File       : retentioninformation.cpp
//
// Description: 
//

#include "retentioninformation.h"

RetentionWindow::RetentionWindow() :
m_startTimeinNSecs(0),m_endTimeinNSecs(0)
{
}

RetentionWindow::RetentionWindow(const RetentionWindow& rhs)
{
    m_startTime = rhs.m_startTime;
    m_startTimeinNSecs = rhs.m_startTimeinNSecs;
    m_endTime = rhs.m_endTime;
    m_endTimeinNSecs = rhs.m_endTimeinNSecs;
}

bool RetentionWindow::operator==(const RetentionWindow& rhs) const
{
    return ((m_startTime == rhs.m_startTime)
    && (m_startTimeinNSecs == rhs.m_startTimeinNSecs)
    && (m_endTime == rhs.m_endTime)
    && (m_endTimeinNSecs == rhs.m_endTimeinNSecs));
}

RetentionWindow& RetentionWindow::operator=(const RetentionWindow& rhs)
{
    if ( this == &rhs)
        return *this;

    m_startTime = rhs.m_startTime;
    m_startTimeinNSecs = rhs.m_startTimeinNSecs;
    m_endTime = rhs.m_endTime;
    m_endTimeinNSecs = rhs.m_endTimeinNSecs;
    
    return *this;
}

RetentionRange::RetentionRange() :
m_startTimeinNSecs(0),m_endTimeinNSecs(0), m_mode(0)
{

}

RetentionRange::RetentionRange(const RetentionRange& rhs)
{
    m_startTime = rhs.m_startTime;
    m_startTimeinNSecs = rhs.m_startTimeinNSecs;
    m_endTime = rhs.m_endTime;
    m_endTimeinNSecs = rhs.m_endTimeinNSecs;
    m_mode = rhs.m_mode;
}

bool RetentionRange::operator==(const RetentionRange& rhs) const
{
    return ( (m_startTime == rhs.m_startTime)
        && (m_startTimeinNSecs == rhs.m_startTimeinNSecs)
        && (m_endTime == rhs.m_endTime)
        && (m_endTimeinNSecs == rhs.m_endTimeinNSecs)
        && (m_mode == rhs.m_mode));
}

RetentionRange& RetentionRange::operator=(const RetentionRange& rhs)
{
    if ( this == &rhs)
        return *this;

    m_startTime = rhs.m_startTime;
    m_startTimeinNSecs = rhs.m_startTimeinNSecs;
    m_endTime = rhs.m_endTime;
    m_endTimeinNSecs = rhs.m_endTimeinNSecs;
    m_mode = rhs.m_mode;

    return *this;
}


RetentionEvent::RetentionEvent() :
m_EventTimeinNSecs(0), m_eventmode(0)
{
}

RetentionEvent::RetentionEvent(const RetentionEvent& rhs)
{
    m_EventTime = rhs.m_EventTime;
    m_EventTimeinNSecs = rhs.m_EventTimeinNSecs;
    m_appName = rhs.m_appName;
    m_EventValue = rhs.m_EventValue;
    m_eventmode = rhs.m_eventmode;
}

bool RetentionEvent::operator==(const RetentionEvent& rhs) const
{
    return ((m_EventTime == rhs.m_EventTime)
    && (m_EventTimeinNSecs == rhs.m_EventTimeinNSecs)
    && (m_appName == rhs.m_appName)
    && (m_EventValue == rhs.m_EventValue)
    && (m_eventmode == rhs.m_eventmode));
}

RetentionEvent& RetentionEvent::operator=(const RetentionEvent& rhs)
{
    if ( this == &rhs)
        return *this;

    m_EventTime = rhs.m_EventTime;
    m_EventTimeinNSecs = rhs.m_EventTimeinNSecs;
    m_appName = rhs.m_appName;
    m_EventValue = rhs.m_EventValue;
    m_eventmode = rhs.m_eventmode;

    return *this;
}

RetentionInformation::RetentionInformation()
{
    m_ranges.clear();
    m_events.clear();
}

RetentionInformation::RetentionInformation(const RetentionInformation& rhs)
{
    m_window = rhs.m_window;
    m_ranges.assign(rhs.m_ranges.begin(), rhs.m_ranges.end());
    m_events.assign(rhs.m_events.begin(), rhs.m_events.end());
}

bool RetentionInformation::operator==(const RetentionInformation& rhs) const
{
    return ((m_window == rhs.m_window)
        && (m_ranges == rhs.m_ranges)
        && (m_events == rhs.m_events));
}

RetentionInformation& RetentionInformation::operator=(const RetentionInformation& rhs)
{
    if ( this == &rhs)
        return *this;

    m_window = rhs.m_window;
    m_ranges.assign(rhs.m_ranges.begin(), rhs.m_ranges.end());
    m_events.assign(rhs.m_events.begin(), rhs.m_events.end());
    
    return *this;
}

CDPEvent::CDPEvent():c_eventtype(0),
	c_eventtime(0),
	c_eventmode(ACCURACYANY),
	c_verified(NONVERIFIEDEVENT),
	c_eventseq(0),
	c_status(CDP_BKMK_INSERT_REQUEST), 
	c_lockstatus(BOOKMARK_STATE_UNLOCKED),
    c_lifetime(0)
{
}

bool CDPEvent::lt_time(const CDPEvent &rhs) const
{
	if((!c_identifier.empty()) && (!rhs.c_identifier.empty()))
	{
		return (c_identifier < rhs.c_identifier);
	}
	else
	{
		return (c_eventtime < rhs.c_eventtime);
	}
}

CDPTimeRange::CDPTimeRange():c_starttime(0),
	c_endtime(0),
	c_mode(ACCURACYEXACT),
	c_startseq(0),
	c_endseq(0),
	c_granularity(0) 
{
}

CDPTimeRange::CDPTimeRange(const SV_ULONGLONG &start,
						   const SV_ULONGLONG & end,
						   SV_USHORT mode,
						   const SV_ULONGLONG & startseq,
						   const SV_ULONGLONG & endseq)
		:c_starttime(start),
		c_endtime(end),
		c_mode(mode),
		c_startseq(startseq),
		c_endseq(endseq)
{
}

bool CDPTimeRange::intersect(CDPTimeRange & rhs) 
{ 
	if(c_mode != rhs.c_mode)
	{
		return  false;
	}

	SV_ULONGLONG lowts, lowseq;
	SV_ULONGLONG hights, highseq;

	if(c_starttime < rhs.c_starttime)
	{
		lowts = rhs.c_starttime;
		lowseq = rhs.c_startseq;
	} else if(c_starttime == rhs.c_starttime)
	{
		lowts = c_starttime;
		lowseq = ((c_startseq > rhs.c_startseq)?c_startseq:rhs.c_startseq);
	} else
	{
		lowts = c_starttime;
		lowseq = c_startseq;
	}

	if(c_endtime < rhs.c_endtime)
	{
		hights = c_endtime;
		highseq = c_endseq;
	} else if(c_endtime == rhs.c_endtime)
	{
		hights = c_endtime;
		highseq = ((c_endseq < rhs.c_endseq)?c_endseq:rhs.c_endseq);
	}
	else
	{
		hights = rhs.c_endtime;
		highseq = rhs.c_endseq;
	}

    // Following change is to address PR# 21985
    
    if( (lowts == hights) && (c_startseq == c_endseq) && ( rhs.c_startseq == rhs.c_endseq) )
    {
        lowseq = ((c_startseq < rhs.c_startseq)? c_startseq: rhs.c_startseq);
        highseq = lowseq;
    }

    // end of change for PR# 21985

	if(lowts > hights)
		return false;
	if( (lowts == hights) && (lowseq > highseq))
		return false;

	c_starttime = lowts;
	c_endtime = hights;
	c_startseq = lowseq;
	c_endseq = highseq;

	return true;
}

bool CDPTimeRange::isempty() 
{ 
	return ((c_starttime > c_endtime) || ((c_starttime == c_endtime)&&(c_startseq > c_endseq))); 
}
