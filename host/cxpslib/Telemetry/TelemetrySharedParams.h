#ifndef TELEMETRY_SHARED_PARAMS_H
#define TELEMETRY_SHARED_PARAMS_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono/system_clocks.hpp>

namespace CxpsTelemetry
{
    // Namespace aliases used in the CxpsTelemetry code
    namespace boost_pt = boost::posix_time;
    namespace chrono = boost::chrono;

    // Typedefs used in the CxpsTelemetry code
    typedef chrono::steady_clock SteadyClock;

    const std::string DIFF_THROTTLE = "Diff-Throttle";
    const std::string RESYNC_THROTTLE = "Resync-Throttle";
    const std::string CUMULATIVE_THROTTLE = "Cumulative-Throttle";
    const std::string THROTTLE_TTL = "TTL";

    class Defaults
    {
    public:
        static boost_pt::ptime s_defaultPTime;
        static SteadyClock::time_point s_defaultTimePoint;
        static chrono::nanoseconds s_zeroTime;
    };

    enum MessageType
    {
        MsgType_Invalid = 0,
        MsgType_Empty = 1,
        MsgType_SourceOpsDiffSync = 2,
        MsgType_SourceOpsResync = 3,
        MsgType_SourceGeneral = 4,
        MsgType_CxpsGlobal = 5,
        // TODO-SanKumar-1711: OnStart, OnStop?!

        MsgType_Count = 6
    };

    enum FileType
    {
        FileType_Invalid = 0,
        FileType_DiffSync = 1,
        FileType_Resync = 2,
        FileType_Log = 3,
        FileType_Telemetry = 4,
        FileType_ChurStat = 5,
        FileType_ThrpStat = 6,
        FileType_TstData = 7,
        FileType_InternalErr = 8,
        FileType_Unknown = 9,

        FileType_Count = 10
    };

    enum DiffSyncFileType
    {
        DiffSyncFileType_Invalid = 0,
        DiffSyncFileType_WriteOrdered = 1,
        DiffSyncFileType_NonWriteOrdered = 2,
        DiffSyncFileType_Tag = 3,
        DiffSyncFileType_Tso = 4,
        DiffSyncFileType_Unknown = 5,
        DiffSyncFileType_InternalErr = 6,

        DiffSyncFileType_Count = 7,

        DiffSyncFileType_All = INT_MAX // Special type used only for printing
    };

    enum ResyncFileType
    {
        ResyncFileType_Invalid = 0,
        ResyncFileType_Sync = 1,
        ResyncFileType_Hcd = 2,
        ResyncFileType_Cluster = 3,
        ResyncFileType_Map = 4,
        ResyncFileType_Unknown = 5,
        ResyncFileType_InternalErr = 6,

        ResyncFileType_Count = 7,

        ResyncFileType_All = INT_MAX // Special type used only for printing
    };

    enum RequestType
    {
        RequestType_Invalid = 0,
        RequestType_PutFile = 1,
        RequestType_ListFile = 2,
        RequestType_RenameFile = 3,
        RequestType_DeleteFile = 4,
        RequestType_GetFile = 5,
        RequestType_Login = 6,
        RequestType_CfsLogin = 7,
        RequestType_FxLogin = 8,
        RequestType_CfsConnectBack = 9,
        RequestType_CfsConnect = 10,
        RequestType_Logout = 11,
        RequestType_Heartbeat = 12,
        RequestType_CfsHeartBeat = 13,

        RequestType_Count = 14
    };

    enum RequestFailure
    {
        RequestFailure_Success = 0,
        RequestFailure_MissingVer,
        RequestFailure_MissingId,
        RequestFailure_MissingReqId,
        RequestFailure_MissingHostId,
        RequestFailure_MissingNonce,
        RequestFailure_MissingFileName,
        RequestFailure_MissingCfsSessionId,
        RequestFailure_MissingCfsSecure,
        RequestFailure_MissingPathsTag,
        RequestFailure_InvalidReqId,
        RequestFailure_UnknownRequest,
        RequestFailure_VerifLogin,
        RequestFailure_VerifLogout,
        RequestFailure_VerifGetFile,
        RequestFailure_VerifDelFile,
        RequestFailure_VerifPutFile,
        RequestFailure_VerifListFile,
        RequestFailure_VerifRenFile,
        RequestFailure_VerifHeartBeat,
        RequestFailure_VerifCfsConnectBack,
        RequestFailure_VerifCfsConnect,
        RequestFailure_NotLoggedIn,
        RequestFailure_BadCnonce,
        RequestFailure_PutFileNoParam,
        RequestFailure_PutFileNoParamVal,
        RequestFailure_PutFileInvalidParams,
        RequestFailure_PutFileParamsReadTimedOut,
        RequestFailure_PutFileParamsFailed,
        RequestFailure_PutFileMissingMoreData,
        RequestFailure_PutFileExpectedMoreData,
        RequestFailure_NoCfsPort,
        RequestFailure_FileAccessDenied,
        RequestFailure_TimedOut,
        RequestFailure_ErrorInResponse,
        RequestFailure_CfsInsecureRequest,
        RequestFailure_HandshakeFailed,
        RequestFailure_HandshakeUnknownError,
        RequestFailure_NwReadFailure,
        RequestFailure_HandleAsyncReadSomeUnknownError,
        RequestFailure_HandleAsyncPutFileUnknownError,
        RequestFailure_ProcessRequestMoreDataExpected,
        RequestFailure_ProtocolHandlerError,
        RequestFailure_ProtocolHandlerUnknownError,
        RequestFailure_RequestHandlerProcessError,
        RequestFailure_PutFileInProgress,
        RequestFailure_LoginFailed,
        RequestFailure_LogoutFailed,
        RequestFailure_FileOpenFailed,
        RequestFailure_FileBadState,
        RequestFailure_FileWriteFailed,
        RequestFailure_FileReadFailed,
        RequestFailure_FileInvalidOffset,
        RequestFailure_NwWriteFailed,
        RequestFailure_HandleAsyncGetFileUnknownError,
        RequestFailure_CompleteCfsConnectFailed,
        RequestFailure_FileOpenInSession,
        RequestFailure_DeleteFileUnknownError,
        RequestFailure_DeleteFileFailed,
        RequestFailure_RenameFileFailed,
        RequestFailure_ListFileFailed,
        RequestFailure_HeartbeatFailed,
        RequestFailure_FileFlushFailed,
        RequestFailure_UnknownError,
        RequestFailure_SessionDestroyed,
        RequestFailure_ServerStopping,
        RequestFailure_MissingOldName,
        RequestFailure_MissingNewName,
        RequestFailure_AcceptError,
        RequestFailure_Deprecated_AsyncAcceptHandlingFailed,
        RequestFailure_AcceptStartSessionFailed,
        RequestFailure_AcceptNewSessionFailed,
        RequestFailure_HandleReadEntireDataFromSocketAsyncUnknownError,
        RequestFailure_CumulativeThrottlingFailure,
        RequestFailure_DiffThrottlingFailure,
        RequestFailure_ResyncThrottlingFailure
    };

    enum RequestTelemetryDataLevel
    {
        ReqTelLevel_Server = 0,
        ReqTelLevel_Session = 1,
        ReqTelLevel_File = 2
    };

    enum RequestTelemetryDataError
    {
        ReqTelErr_None = 0,
        ReqTelErr_TimerAlreadyStarted,
        ReqTelErr_TimerNotStarted,
        ReqTelErr_AnotherOpInProgress,
        ReqTelErr_UnexpectedOpInProgress,
        ReqTelErr_DanglingCurrOp,
        ReqTelErr_NumReqsLE0,

        ReqTelErr_Count
    };

    enum TelemetryFailure
    {
        TelFailure_None = 0,
        TelFailure_Alloc,
        TelFailure_AddToDiffTel,
        TelFailure_AddToResyncTel,
        TelFailure_AddToSrcGenTel,
        TelFailure_AddToGlobalTel,
        TelFailure_DiffEmptyMeta,
        TelFailure_ResyncEmptyMeta,
        TelFailure_AddTelUnknFType,
        TelFailure_SrcGenEmptyMeta,
        TelFailure_AddTelReachedEof,
        TelFailure_AddToTelUnknErr,

        TelFailure_Count
    };
}
#endif //TELEMETRY_SHARED_PARAMS_H