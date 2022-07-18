///
/// @file svdparse.h
///
/// Define interface to SV Delta files. Delta files are a list of chunks.
/// Each chunk begins with a four character tag,
/// a length, and then that many data bytes. 
///
#ifndef SVDPARSE__H
#define SVDPARSE__H

#include "svtypes.h"

#define BYTEMASK(ch)    ((ch) & (0xFF))
#define INMAGE_MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
    (BYTEMASK(ch0) | (BYTEMASK(ch1) << 8) |   \
    (BYTEMASK(ch2) << 16) | (BYTEMASK(ch3) << 24 ))



#define SVD_VERSION(major,minor)     (((major) << 8) + (minor))
#define SVD_BASEVERSION              SVD_VERSION(0,0)

#define SV_WO_STATE_POS                     1u
#define SV_PER_IO_POS                       1u
#define SV_MIRROR_POS                       2u
#define SV_DIRTYBLK_NUM_POS                 2u
#define SV_DEVICE_STATS_NUM_POS             1u
#define SV_MAX_TRANSFER_SIZE_NUM_POS        2u
#define SV_CX_EVENT_SUPPORT_POS             4u

#define SV_TAG_COMMIT_NOTIFY_SUPPORT_POS    8u
#define SV_TAG_BLOCK_DRAIN_SUPPORT_POS      32u

#define SVD_PERIOVERSION_MAJOR  2
#define SVD_PERIOVERSION_MINOR  0
#define SVD_PERIOVERSION             SVD_VERSION(SVD_PERIOVERSION_MAJOR,SVD_PERIOVERSION_MINOR)

#define SV_DRIVER_VERSION(a, b, c, d)	(((a) << 24) + ((b) << 16) +  ((c) << 8)  + (d))
#define SV_DRIVER_PERIOVERSION          SV_DRIVER_VERSION(2,1,0,0)
#define SV_DRIVER_INITIAL_VERSION          SV_DRIVER_VERSION(2,0,0,0)

// prefix for SVD_HEADER1 structure
#define SVD_TAG_HEADER1                  INMAGE_MAKEFOURCC( 'S', 'V', 'D', '1' )

//
// prefix for SVD_HEADER2 structure
// SVD_HEADER2 contains version field in addition to fields present in SVD_HEADER1
// version field was introduced to inform the driver version to target site
// it was introduced to communicate whether per i/o timestamps changes are part
// of differential file
// however, this tag/structure is not being populated from source
// instead per i/o version is based on timestamp tag

#define SVD_TAG_HEADER_V2                INMAGE_MAKEFOURCC( 'S', 'V', 'D', '2' )


// TAG: TSFC - prefix for SVD_TIME_STAMP structure to communicate
//             timestamp of first change

#define SVD_TAG_TIME_STAMP_OF_FIRST_CHANGE  INMAGE_MAKEFOURCC( 'T', 'S', 'F', 'C' )

// TAG: TFV2 - prefix for SVD_TIME_STAMP_V2 structure to communicate
//             timestamp of first change

#define SVD_TAG_TIME_STAMP_OF_FIRST_CHANGE_V2  INMAGE_MAKEFOURCC( 'T', 'F', 'V', '2' )

// TAG: TSLC - prefix for SVD_TIME_STAMP structure to communicate
//             timestamp of last change

#define SVD_TAG_TIME_STAMP_OF_LAST_CHANGE   INMAGE_MAKEFOURCC( 'T', 'S', 'L', 'C' )

// TAG: TLV2 - prefix for SVD_TIME_STAMP_V2 structure to communicate
//             timestamp of last change

#define SVD_TAG_TIME_STAMP_OF_LAST_CHANGE_V2   INMAGE_MAKEFOURCC( 'T', 'L', 'V', '2' )

// TAG: USER - prefix for communicating one bookmark field
//             flags field contains the length of bookmark field
#define SVD_TAG_USER                     INMAGE_MAKEFOURCC( 'U', 'S', 'E', 'R' )

// TAG: LODC - prefix for SV_ULARGE_INTEGER containing length to skip for reaching TSLC/
//             TLV2 prefix
//             in case of cdp files, this is not being used (see TAG: DATA)
#define SVD_TAG_LENGTH_OF_DRTD_CHANGES      INMAGE_MAKEFOURCC( 'L', 'O', 'D', 'C' )

// TAG: DIRT - obsolete
// TAG: SENT - obsolete
// TAG: SDRT - obsolete

#define SVD_TAG_DIRTY_BLOCKS             INMAGE_MAKEFOURCC( 'D', 'I', 'R', 'T' )
#define SVD_TAG_SENTINEL_HEADER          INMAGE_MAKEFOURCC( 'S', 'E', 'N', 'T' )
#define SVD_TAG_SENTINEL_DIRT            INMAGE_MAKEFOURCC( 'S', 'D', 'R', 'T' )


// TAG: SHCD - used by fast sync for communicating hash compare for a chunk of data
// TAG: SDAT - unused
// TAG: SDNI - obsolete (earlier used for sending mismatch block info to source)
// see dataprotection/hascomparedata.h, syncdataneededinfo.h for corresponding structures

#define SVD_TAG_SYNC_HASH_COMPARE_DATA   INMAGE_MAKEFOURCC( 'S', 'H', 'C', 'D' )
#define SVD_TAG_SYNC_DATA                INMAGE_MAKEFOURCC( 'S', 'D', 'A', 'T' )
#define SVD_TAG_SYNC_DATA_NEEDED_INFO    INMAGE_MAKEFOURCC( 'S', 'D', 'N', 'I' )

// TAG: DRTD - used as prefix for SVD_DIRTY_BLOCK structure, used when only change 
//             is to be communicated without timestamp information 
//             e.g: fast sync, agents without per i/o implementation
// TAG: DDV2 - used as prefix for SVD_DIRTY_BLOCK_V2 containing change
//             along with timestamp 
#define SVD_TAG_DIRTY_BLOCK_DATA         INMAGE_MAKEFOURCC( 'D', 'R', 'T', 'D' )
#define SVD_TAG_DIRTY_BLOCK_DATA_V2      INMAGE_MAKEFOURCC( 'D', 'D', 'V', '2' )

//for solaris endianness changes...
#define SVD_TAG_BEFORMAT INMAGE_MAKEFOURCC('D','R','T','B') 
#define SVD_TAG_LEFORMAT INMAGE_MAKEFOURCC('D','R','T','L')

// TAGS introduded for cdp module
// 
// Note: 
//  DATA, DDV3, SKIP introduced as part of asynchronous i/o and no-buffering
//  changes in 5.0/5.1; 
//
// 
// TAG: DATA - prefix for SVD_DATA_INFO strucure
// TAG: DDV3 - after this tag, follows list of SVD_DIRTY_BLOCK_V2 structures
//             count field containing no. of such structures
//             actual data is stored after the change information
// TAG: SKIP - after this tag, follows white space to make sure the complete
//             metadata is multiple of 512 (sector) bytes. 

#define SVD_TAG_DATA_INFO                INMAGE_MAKEFOURCC( 'D', 'A', 'T', 'A' )
#define SVD_TAG_DIRTY_BLOCK_DATA_V3      INMAGE_MAKEFOURCC( 'D', 'D', 'V', '3' )
#define SVD_TAG_SKIP                     INMAGE_MAKEFOURCC( 'S', 'K', 'I', 'P' )


// TAGS introduced for cdp module
// as part of sparse retention
//  
// TAG: OTHR - prefix for SVD_OTHR_INFO
//
// TAG: FOST  - after this tag follows a list of 
//                 {retention fileid, start offset}
//              structure: SVD_FOST_INFO
//              count field tells no. of such strucures
//
// TAG: CDPD - after this tag follows a list of CDP_DRTD structures
//             count field tells no. of such structures
//
// TAG: SOST - after this tag, SV_ULONGLONG field provides offset to
//             reach start of the metadata
//
#define SVD_TAG_OTHR                     INMAGE_MAKEFOURCC( 'O', 'T', 'H', 'R' )
#define SVD_TAG_FOST                     INMAGE_MAKEFOURCC( 'F', 'O', 'S', 'T' )
#define SVD_TAG_CDPD                     INMAGE_MAKEFOURCC( 'C', 'D', 'P', 'D' )
#define SVD_TAG_SOST                     INMAGE_MAKEFOURCC( 'S', 'O', 'S', 'T' )

#define SVD_FOST_FNAME_SIZE 56

#if defined(SV_SUN) || defined(SV_AIX)
    #pragma pack( 1 )
#else
    #pragma pack( push, 1 )
#endif /* SV_SUN  || SV_AIX */

struct SVD_PREFIX
{
    SV_UINT tag;
    SV_UINT count;
    SV_UINT Flags;
};

// TAG: SVD1
struct SVD_HEADER1
{
    SV_UCHAR  MD5Checksum[16];        // MD5 checksum of all data that follows this field
    SV_GUID   SVId;                   // Unique ID assigned by amethyst
    SV_GUID   OriginHost;             // Unique origin host id
    SV_GUID   OriginVolumeGroup;      // Unique origin vol group id
    SV_GUID   OriginVolume;           // Unique origin vol id
    SV_GUID   DestHost;               // Unique dest host id
    SV_GUID   DestVolumeGroup;        // Unique dest vol group id
    SV_GUID   DestVolume;             // Unique dest vol id
};

// TAG: SVD2
struct SVD_VERSION_INFO
{
    SV_UINT	    Major;
    SV_UINT 	Minor;
};

struct SVD_HEADER_V2
{
    SVD_VERSION_INFO Version;
    SV_UCHAR  MD5Checksum[16];        // MD5 checksum of all data that follows this field
    SV_GUID   SVId;                   // Unique ID assigned by amethyst
    SV_GUID   OriginHost;             // Unique origin host id
    SV_GUID   OriginVolumeGroup;      // Unique origin vol group id
    SV_GUID   OriginVolume;           // Unique origin vol id
    SV_GUID   DestHost;               // Unique dest host id
    SV_GUID   DestVolumeGroup;        // Unique dest vol group id
    SV_GUID   DestVolume;             // Unique dest vol id
};

// FIXME:
// these are defined in invalflt.h, but should not be 
// as such it causes compile issue so for now
// redefine them here and make sure to keep in sync with driver 
// typedef struct _TIME_STAMP_TAG
//{
//    STREAM_REC_HDR_4B   Header;
//    ULONG       ulSequenceNumber;
//    ULONGLONG   TimeInHundNanoSecondsFromJan1601;
//} TIME_STAMP_TAG, *PTIME_STAMP_TAG;

struct SVD_TIME_STAMP_HEADER 
{
    SV_USHORT      usStreamRecType;
    SV_UCHAR       ucFlags;
    SV_UCHAR       ucLength;
};

// TAG: TSFC, TSLC
struct SVD_TIME_STAMP
{
    SVD_TIME_STAMP_HEADER   Header;
    SV_UINT                ulSequenceNumber;
    SV_ULONGLONG            TimeInHundNanoSecondsFromJan1601;
};

// TAG: TFV2, TLV2
struct SVD_TIME_STAMP_V2
{
    SVD_TIME_STAMP_HEADER   Header;
    SV_ULONGLONG            SequenceNumber;
    SV_ULONGLONG            TimeInHundNanoSecondsFromJan1601;
};


// obsolete
// TAG: DIRT
// Doesn't exist anymore. Just an array of SVD_DIRTY_BLOCK followed by Length data bytes. 
struct SVD_DIRTY_BLOCK_DATA
{
    SV_LONGLONG BlockCount;
};

// obsolete
// TAG: SENT,SDRT
// Raw sentinel dirt file header. Two blocks: SENT and SDRT
//
struct SENTINEL_DIRTYFILE_HEADER
{
    SV_ULONG tagSentinelHeader;
    SV_ULONG dwSize;
    SV_ULONG dwMajorVersion, dwMinorVersion;
    SV_ULONGLONG ullVolumeCapacity;
    SV_ULONG dwPageSize;
    SV_ULONG tagSentinelDirty;
    SV_ULONG dwDirtSize;
};

//  obsolete/unused
struct SVD_BLOCK_CHECKSUM
{
    SV_LONGLONG         Length;
    SV_LONGLONG         ByteOffset;
    SV_UCHAR            MD5Checksum[16];  // MD5 checksum of all data that follows this field
};


// TAG: DRTD
struct SVD_DIRTY_BLOCK
{
    SV_LONGLONG         Length;
    SV_LONGLONG         ByteOffset;
};

// TAG: DDV2
struct SVD_DIRTY_BLOCK_V2
{
    SV_UINT    	    Length;
    SV_ULONGLONG	ByteOffset;
    SV_UINT      	uiSequenceNumberDelta;
    SV_UINT      	uiTimeDelta;
};


// TAG: DATA (for cdp module without sparse)
struct SVD_DATA_INFO
{
    SV_OFFSET_TYPE    drtdStartOffset;
    SV_ULARGE_INTEGER drtdLength;
    SV_OFFSET_TYPE    diffEndOffset;
};

// TAG: OTHR (cdp module, sparse retention)
struct SVD_OTHR_INFO
{
	SV_UINT  type;
};

// TAG: FOST (cdp module, sparse retention)
struct SVD_FOST_INFO
{
	char filename[SVD_FOST_FNAME_SIZE];
	SV_OFFSET_TYPE startoffset;
};

// TAG: CDPD (cdp module, sparse retention)
// each drtd is of fixed 4k block size
struct SVD_CDPDRTD
{
	SV_OFFSET_TYPE  voloffset;
	SV_UINT         length;
	SV_UINT         fileid;
	SV_UINT      	uiTimeDelta;
	SV_UINT      	uiSequenceNumberDelta;
};


// Note: this is not part of differential file
// it is used by vsnap btree code for btree nodes
struct SVD_DIRTY_BLOCK_INFO
{
    struct SVD_DIRTY_BLOCK DirtyBlock;
    SV_ULONGLONG DataFileOffset;
};




#define SVD_PREFIX_SIZE	       sizeof(SVD_PREFIX)
#define SVD_HEADER1_SIZE       sizeof(SVD_HEADER1)
#define SVD_HEADER2_SIZE       sizeof(SVD_HEADER_V2)
#define SVD_VERSION_SIZE       sizeof(SVD_VERSION_INFO)
#define SVD_TIMESTAMP_SIZE     sizeof(SVD_TIME_STAMP)
#define SVD_TIMESTAMP_V2_SIZE  sizeof(SVD_TIME_STAMP_V2)
#define SVD_LODC_SIZE          sizeof(SV_ULARGE_INTEGER)
#define SVD_DIRTY_BLOCK_SIZE   sizeof(SVD_DIRTY_BLOCK)
#define SVD_DIRTY_BLOCK_V2_SIZE   sizeof(SVD_DIRTY_BLOCK_V2)
#define SVD_DATA_INFO_SIZE     sizeof(SVD_DATA_INFO)
#define SVD_OTHR_INFO_SIZE     sizeof(SVD_OTHR_INFO)
#define SVD_FOST_INFO_SIZE     sizeof(SVD_FOST_INFO)
#define SVD_CDPDRTD_SIZE      sizeof(SVD_CDPDRTD)
#define SVD_SKIP_SIZE         sizeof(SV_UINT)
#define SVD_SOST_SIZE          sizeof(SV_OFFSET_TYPE)

#define SVD_TAG_HEADER1_NAME             "SVD1"
#define SVD_TAG_HEADER2_NAME             "SVD2"
#define SVD_TAG_TSFC_NAME                "TSFC"
#define SVD_TAG_TSFC_V2_NAME             "TFV2"
#define SVD_TAG_TSLC_NAME                "TSLC"
#define SVD_TAG_TSLC_V2_NAME             "TLV2"
#define SVD_TAG_USER_NAME                "USER"
#define SVD_TAG_LODC_NAME                "LODC"
#define SVD_TAG_DATA_INFO_NAME           "DATA"
#define SVD_TAG_DIRTY_BLOCK_DATA_NAME    "DRTD"
#define SVD_TAG_DIRTY_BLOCK_DATA_V2_NAME "DDV2"
#define SVD_TAG_DIRTY_BLOCK_DATA_V3_NAME "DDV3"
#define SVD_TAG_SKIP_NAME                "SKIP"
#define SVD_TAG_OTHR_NAME                "OTHR"
#define SVD_TAG_FOST_NAME                "FOST"
#define SVD_TAG_CDPD_NAME                "CDPD"
#define SVD_TAG_SOST_NAME                "SOST"

#define FILL_PREFIX(pf,tg,cnt,flgs)       { pf.tag = tg; pf.count = cnt; pf.Flags=flgs; }

/*SRM: start*/

#define SVD_TAG_REDOLOG_HEADER           INMAGE_MAKEFOURCC( 'R', 'D', 'L', 'F' )

struct SVD_REDOLOG_HEADER
{
	SVD_VERSION_INFO Version;
};

#define SVD_REDOLOG_HEADER_SIZE sizeof(SVD_REDOLOG_HEADER)
#define SVD_TAG_REDOLOG_HEADER_NAME "RDLF"

/*SRM: end*/

#if defined(SV_SUN) || defined(SV_AIX)
    #pragma pack()
#else
    #pragma pack( pop )
#endif /* SV_SUN || SV_AIX */

#endif // SVDPARSE__H
