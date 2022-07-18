//
// Copyright (c) 2005 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File       :retentioninformation.h
//
// Description: 
//

#ifndef RETENTIONINFORMATION__H
#define RETENTIONINFORMATION__H

#include "svtypes.h"
#include <string>
#include <list>
#include <map>

#define ACCURACYEXACT           0
#define ACCURACYAPPR            1
#define ACCURACYNOTGAURANTEED   2
#define ACCURACYANY             3

#define NONVERIFIEDEVENT	0
#define VERIFIEDEVENT		1
#define BOTH_VERIFIED_NONVERIFIED_EVENT	2

#define BOOKMARK_STATE_UNLOCKED			0
#define BOOKMARK_STATE_LOCKEDBYUSER		1
#define BOOKMARK_STATE_LOCKEDBYVSNAP	2
#define BOOKMARK_STATE_BOTHLOCKED_UNLOCKED	3
#define BOOKMARK_STATE_LOCKED			4

#define CDP_BKMK_INSERT_REQUEST  0
#define CDP_BKMK_UPDATE_REQUEST  1
#define CDP_BKMK_DELETE_REQUEST  2

#define CDP_TIMERANGE_INSERT_REQUEST  0
#define CDP_TIMERANGE_DELETE_REQUEST  1

// defines for granularity

#define CDP_ANY_POINT_IN_TIME 0
#define CDP_ONLY_END_POINTS  1

typedef std::map<SV_EVENT_TYPE, SV_ULONGLONG> EventSummary_t;

struct CDPSummary
{
	SV_ULONGLONG start_ts;
	SV_ULONGLONG end_ts;
	SV_ULONGLONG free_space;
	double version;
	double revision;
	SV_USHORT log_type;
	EventSummary_t event_summary;
	CDPSummary()
	{
		start_ts = 0;
		end_ts   = 0;
		free_space = 0;
		version = 0;
		revision = 0;
		log_type = 0;
	}
};

struct CDPEvent
{
	SV_ULONGLONG c_eventno;
	SV_USHORT    c_eventtype;
	SV_ULONGLONG c_eventtime;
	SV_ULONGLONG c_eventseq;
	std::string  c_eventvalue;
	SV_USHORT	 c_eventmode;
	std::string  c_identifier;
	SV_USHORT	 c_verified;
	std::string  c_comment;
	SV_USHORT    c_status;
	SV_USHORT	 c_lockstatus;
    SV_ULONGLONG c_lifetime;
	
	CDPEvent();
	
	bool lt_time(CDPEvent const& rhs) const;
};
typedef std::list<CDPEvent> CDPEvents;

struct CDPTimeRange
{
	SV_ULONGLONG c_entryno;
	SV_ULONGLONG c_starttime;
	SV_ULONGLONG c_startseq;
	SV_ULONGLONG c_endtime;
	SV_ULONGLONG c_endseq;
	SV_USHORT c_mode;
	SV_USHORT c_granularity;
	SV_USHORT c_action;
	

	CDPTimeRange();
	CDPTimeRange(const SV_ULONGLONG &start, const SV_ULONGLONG & end, SV_USHORT mode, const SV_ULONGLONG & startseq, const SV_ULONGLONG & endseq);
	SV_ULONGLONG low() { return c_starttime ;}
	SV_ULONGLONG high() { return c_endtime ; }
	SV_ULONGLONG mode() { return c_mode ; }
	SV_ULONGLONG startseq() { return c_startseq ; }
	SV_ULONGLONG endseq()   { return c_endseq ; }
	bool isAccurate()  { return (c_mode == ACCURACYEXACT) ; }
	bool intersect(CDPTimeRange & rhs);
	bool isempty(); 
};


typedef std::list<CDPTimeRange> CDPTimeRanges;

struct CDPInformation
{
	CDPSummary    m_summary;
	CDPTimeRanges m_ranges;
	CDPEvents     m_events;
};

typedef std::map<std::string,CDPInformation> HostCDPInformation;

#define CDPV1_SPARSE_SAVINGS_NOTAPPLICABLE -1
struct CDPRetentionDiskUsage
{
    SV_ULONGLONG start_ts;
	SV_ULONGLONG end_ts;
    SV_ULONGLONG num_files;
	SV_ULONGLONG size_on_disk;
    SV_ULONGLONG space_saved_by_compression;
	SV_LONGLONG space_saved_by_sparsepolicy;
	CDPRetentionDiskUsage()
	{
		start_ts = 0;
		end_ts   = 0;
		num_files = 0;
		size_on_disk = 0;
		space_saved_by_compression = 0;
		space_saved_by_sparsepolicy = 0;
	}
};

struct CDPTargetDiskUsage
{
	SV_ULONGLONG size_on_disk;
	SV_ULONGLONG space_saved_by_thinprovisioning;
	SV_ULONGLONG space_saved_by_compression; //unused for this version

	CDPTargetDiskUsage()
	{
		size_on_disk = 0;
		space_saved_by_thinprovisioning = 0;
		space_saved_by_compression = 0;
	}
};


typedef std::map<std::string, CDPRetentionDiskUsage>  HostCDPRetentionDiskUsage;
typedef std::map<std::string, CDPTargetDiskUsage>     HostCDPTargetDiskUsage;

// Php names
// updateCdpInformationV2
// updateCdpDiskUsage

struct RetentionWindow
{
	std::string m_startTime;			// human readable format
	SV_ULONGLONG m_startTimeinNSecs;	// in machine format in hundred nano seconds
	std::string m_endTime;				// human readable format
	SV_ULONGLONG m_endTimeinNSecs;		// in machine format in hundred nano seconds

    RetentionWindow();
    RetentionWindow(const RetentionWindow&);
    bool operator==(const RetentionWindow&) const;
    RetentionWindow& operator=(const RetentionWindow&);
};

struct RetentionRange
{
	std::string m_startTime;			// human readable format
	SV_ULONGLONG m_startTimeinNSecs;	// in machine format in hundred nano seconds
	std::string m_endTime;				// human readable format
	SV_ULONGLONG m_endTimeinNSecs;		// in machine format in hundred nano seconds
	SV_UINT m_mode;				// accuract mode (exact,approximate, not guaranteed)

    RetentionRange();
    RetentionRange(const RetentionRange&);
    bool operator==(const RetentionRange&) const;
    RetentionRange& operator=(const RetentionRange&);
};

typedef std::list<RetentionRange> RetentionRanges;

struct RetentionEvent
{
	std::string m_EventTime;			// human readable format
	SV_ULONGLONG m_EventTimeinNSecs;	// in machine format in hundred nano seconds
	std::string m_appName;              // application name
	std::string m_EventValue;           // vacp tag issued by user
	SV_UINT m_eventmode;                // accuracy mode (exact, approximate, out of sync)

    RetentionEvent();
    RetentionEvent(const RetentionEvent&);
    bool operator==(const RetentionEvent&) const;
    RetentionEvent& operator=(const RetentionEvent&);
};

typedef std::list<RetentionEvent> RetentionEvents;

struct RetentionInformation
{
	RetentionWindow m_window;
	RetentionRanges m_ranges;
	RetentionEvents m_events;

    RetentionInformation();
    RetentionInformation(const RetentionInformation&);
    bool operator==(const RetentionInformation&) const;
    RetentionInformation& operator=(const RetentionInformation&);
};

typedef std::string volumeName;
typedef std::map<volumeName,RetentionWindow> HostRetentionWindow;
typedef std::map<volumeName,RetentionInformation> HostRetentionInformation;

#endif
