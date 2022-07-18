#ifndef CXPS_TEL_STRINGER_H
#define CXPS_TEL_STRINGER_H

#include "TelemetryCommon.h"

#define STR(name) static const std::string name(#name)
#define DEF_MDS_COL_NAME(name) STR(name)

namespace CxpsTelemetry
{
    namespace Strings
    {
        // NOTE: Keep the strings as short as possible, as they are redundant
        // data (key values) transported over customer's internet.

        namespace MdsColumnNames
        {
            DEF_MDS_COL_NAME(BiosId);
            DEF_MDS_COL_NAME(FabType);
            DEF_MDS_COL_NAME(PSHostId);
            DEF_MDS_COL_NAME(PSAgentVer);
            DEF_MDS_COL_NAME(SeqNum);
            DEF_MDS_COL_NAME(MessageType);
            DEF_MDS_COL_NAME(Message);
            DEF_MDS_COL_NAME(PendingReqInfo);
            DEF_MDS_COL_NAME(HostId);
            DEF_MDS_COL_NAME(Device);
            DEF_MDS_COL_NAME(LoggerStartTime);
            DEF_MDS_COL_NAME(LoggerEndTime);
            DEF_MDS_COL_NAME(ReqsStartTime);
            DEF_MDS_COL_NAME(ReqsEndTime);
            DEF_MDS_COL_NAME(CustomJson);
        }

        namespace DynamicJson
        {
            STR(Pid);
            STR(Data);

            namespace FileTelemetryData
            {
                STR(TotReqs);
                STR(SuccReqs);
                STR(FailedReqs);
                STR(TotFiles);
                STR(SuccFiles);
                STR(FailedFiles);
                STR(TotNewReqBlockTime);
                STR(TotReqOpTime);
                STR(Failures);
                STR(TotReqTime);
            }

            namespace FileTelemetryDataCollection
            {
                STR(ReqType);
                STR(FileType);
                STR(FSubType);
                STR(MdRange);
                STR(Telemetry);
            }

            namespace PerfCounters
            {
                STR(TotFWriteTime);
                STR(TotFReadTime);
                STR(TotNwReadTime);
                STR(TotNwWriteTime);
                STR(TotFFlushTime);
                STR(TotFComprTime);
                STR(TotFWriteCnt);
                STR(TotFReadCnt);
                STR(TotNwReadCnt);
                STR(TotNwWriteCnt);
                STR(SuccFWriteCnt);
                STR(SuccFReadCnt);
                STR(SuccNwReadCnt);
                STR(SuccNwWriteCnt);
                STR(FailedFWriteCnt);
                STR(FailedFReadCnt);
                STR(FailedNwReadCnt);
                STR(FailedNwWriteCnt);
                STR(TotNwReadBytes);
                STR(TotNwWriteBytes);
                STR(TotFReadBytes);
                STR(TotFWriteBytes);
                STR(TotPutInterReqTime);
                STR(NwReadBuckets);
                STR(FWriteBuckets);
            }

            namespace SourceOpsDiffSync
            {
                namespace FileMetadata
                {
                    STR(FirstPrevEndTS);
                    STR(FirstPrevSeqNum);
                    STR(FirstSplitIoSeqId);
                    STR(LastTS);
                    STR(LastSeqNum);
                    STR(LastSplitIoSeqId);
                }
            }

            namespace SourceOpsResync
            {
                namespace FileMetadata
                {
                    STR(FirstJobId);
                    STR(FirstTS);
                    STR(LastJobId);
                    STR(LastTS);
                }
            }

            namespace SourceGeneral
            {
                STR(PerReqData);
                STR(PerFTypeData);
            }

            namespace GlobalTel
            {
                STR(MsgDrops);
                STR(TelDataErr);
                STR(TelFailures);
                STR(ReqFailures);
            }
        }

        namespace MessageType
        {
            STR(Msg_Inv);
            STR(Empty);
            STR(Diff);
            STR(Resync);
            STR(SrcGen);
            STR(Global);
            STR(Msg_Def);
        }

        namespace SourceFileType
        {
            STR(DiffSync);
            STR(Resync);
            STR(Log);
            STR(Telemetry);
            STR(ChurStat);
            STR(ThrpStat);
            STR(TstData);

            STR(FileType_Def);
            STR(FileType_Inv);
            STR(FileType_IntErr);
            STR(FileType_Unkn);
        }

        namespace ResyncFileType
        {
            STR(Map);
            STR(Sync);
            STR(Hcd);
            STR(Cluster);

            STR(Resync_Inv);
            STR(Resync_Unkn);
            STR(Resync_IntErr);
            STR(Resync_Def);

            STR(Resync_All);
        }

        namespace DiffSyncFileType
        {
            STR(Wo);
            STR(Nwo);
            STR(Tag);
            STR(Tso);

            STR(Diff_Inv);
            STR(Diff_Unkn);
            STR(Diff_IntErr);
            STR(Diff_Def);

            STR(Diff_All);
        }

        namespace RequestType
        {
            STR(Req_Def);
            STR(Req_Inv);
            STR(PutFile);
            STR(LsFile);
            STR(RenFile);
            STR(DelFile);
            STR(GetFile);
            STR(Login);
            STR(CfsLogin);
            STR(FxLogin);
            STR(CfsConnBack);
            STR(CfsConn);
            STR(Logout);
            STR(Hbeat);
            STR(CfsHbeat);
        }

        namespace ReqFailType
        {
            STR(ReqFail_Def);
            STR(Success);
            STR(MissingVer);
            STR(MissingId);
            STR(MissingReqId);
            STR(MissingHostId);
            STR(MissingNonce);
            STR(MissingFileName);
            STR(MissingCfsSessionId);
            STR(MissingCfsSecure);
            STR(MissingPathsTag);
            STR(InvalidReqId);
            STR(UnknownRequest);
            STR(VerifLogin);
            STR(VerifLogout);
            STR(VerifGetFile);
            STR(VerifDelFile);
            STR(VerifPutFile);
            STR(VerifListFile);
            STR(VerifRenFile);
            STR(VerifHeartBeat);
            STR(VerifCfsConnectBack);
            STR(VerifCfsConnect);
            STR(NotLoggedIn);
            STR(BadCnonce);
            STR(PutFileNoParam);
            STR(PutFileNoParamVal);
            STR(PutFileInvalidParams);
            STR(PutFileParamsReadTimedOut);
            STR(PutFileParamsFailed);
            STR(PutFileMissingMoreData);
            STR(PutFileExpectedMoreData);
            STR(NoCfsPort);
            STR(FileAccessDenied);
            STR(TimedOut);
            STR(ErrorInResponse);
            STR(CfsInsecureRequest);
            STR(HandshakeFailed);
            STR(HandshakeUnknownError);
            STR(NwReadFailure);
            STR(HandleAsyncReadSomeUnknownError);
            STR(HandleAsyncPutFileUnknownError);
            STR(ProcessRequestMoreDataExpected);
            STR(ProtocolHandlerError);
            STR(ProtocolHandlerUnknownError);
            STR(RequestHandlerProcessError);
            STR(PutFileInProgress);
            STR(LoginFailed);
            STR(LogoutFailed);
            STR(FileOpenFailed);
            STR(FileBadState);
            STR(FileWriteFailed);
            STR(FileReadFailed);
            STR(FileInvalidOffset);
            STR(NwWriteFailed);
            STR(HandleAsyncGetFileUnknownError);
            STR(CompleteCfsConnectFailed);
            STR(FileOpenInSession);
            STR(DeleteFileUnknownError);
            STR(DeleteFileFailed);
            STR(RenameFileFailed);
            STR(ListFileFailed);
            STR(HeartbeatFailed);
            STR(FileFlushFailed);
            STR(UnknownError);
            STR(SessionDestroyed);
            STR(ServerStopping);
            STR(MissingOldName);
            STR(MissingNewName);
            STR(AcceptErr);
            STR(Deprecated_AsyncAccHandlFailed);
            STR(AcceptStartSessionFailed);
            STR(AcceptNewSessionFailed);
            STR(HandleReadEntireDataFromSocketAsyncUnknownError);
            STR(CumulativeThrottlingFailure);
            STR(DiffThrottlingFailure);
            STR(ResyncThrottlingFailure);
        }

        namespace TelemetryDataError
        {
            STR(None);
            STR(TimerStarted);
            STR(TimerNStarted);
            STR(AnothOpInProg);
            STR(UnexpOpInProg);
            STR(DangCurrOp);
            STR(NumReqsLE0);

            STR(TelDatErr_Def);
        }

        namespace TelemetryFailures
        {
            STR(None);
            STR(Alloc);
            STR(AddToDiffTel);
            STR(AddToResyncTel);
            STR(AddToSrcGenTel);
            STR(AddToGlobalTel);
            STR(DiffEmptyMeta);
            STR(ResyncEmptyMeta);
            STR(AddTelUnknFType);
            STR(SrcGenEmptyMeta);
            STR(AddTelReachedEof);
            STR(AddToTelUnknErr);

            STR(TelFail_Def);
        }
    }

    class Stringer
    {

    public:

#undef MAP
#define MAP(msgType, strObj) case msgType: return Strings::MessageType::strObj

        static const std::string& MessageTypeToString(MessageType messageType)
        {
            switch (messageType)
            {
                MAP(MsgType_Empty, Empty);
                MAP(MsgType_SourceOpsDiffSync, Diff);
                MAP(MsgType_SourceOpsResync, Resync);
                MAP(MsgType_SourceGeneral, SrcGen);
                MAP(MsgType_CxpsGlobal, Global);
                MAP(MsgType_Invalid, Msg_Inv);
            default:
                BOOST_ASSERT(false);
                return Strings::MessageType::Msg_Def;
            }
        }

#undef MAP
#define MAP(srcFileType, strObj) case srcFileType: return Strings::SourceFileType::strObj

        static const std::string& SourceFileTypeToString(FileType fileType)
        {
            switch (fileType)
            {
                MAP(FileType_Invalid, FileType_Inv);
                MAP(FileType_DiffSync, DiffSync);
                MAP(FileType_Resync, Resync);
                MAP(FileType_Log, Log);
                MAP(FileType_Telemetry, Telemetry);
                MAP(FileType_ChurStat, ChurStat);
                MAP(FileType_ThrpStat, ThrpStat);
                MAP(FileType_TstData, TstData);
                MAP(FileType_InternalErr, FileType_IntErr);
                MAP(FileType_Unknown, FileType_Unkn);
            default:
                BOOST_ASSERT(false);
                return Strings::SourceFileType::FileType_Def;
            }
        }

#undef MAP
#define MAP(diffFileType, strObj) case diffFileType: return Strings::DiffSyncFileType::strObj

        static const std::string& DiffSyncFileTypeToString(DiffSyncFileType diffFileType)
        {
            switch (diffFileType)
            {
                MAP(DiffSyncFileType_All, Diff_All); // Used in printing only
                MAP(DiffSyncFileType_Invalid, Diff_Inv);
                MAP(DiffSyncFileType_WriteOrdered, Wo);
                MAP(DiffSyncFileType_NonWriteOrdered, Nwo);
                MAP(DiffSyncFileType_Tag, Tag);
                MAP(DiffSyncFileType_Tso, Tso);
                MAP(DiffSyncFileType_Unknown, Diff_Unkn);
                MAP(DiffSyncFileType_InternalErr, Diff_IntErr);
            default:
                BOOST_ASSERT(false);
                return Strings::DiffSyncFileType::Diff_Def;
            }
        }

#undef MAP
#define MAP(resyncFileType, strObj) case resyncFileType: return Strings::ResyncFileType::strObj

        static const std::string& ResyncFileTypeToString(ResyncFileType resyncFileType)
        {
            switch (resyncFileType)
            {
                MAP(ResyncFileType_All, Resync_All); // Used in printing only
                MAP(ResyncFileType_Invalid, Resync_Inv);
                MAP(ResyncFileType_Sync, Sync);
                MAP(ResyncFileType_Hcd, Hcd);
                MAP(ResyncFileType_Cluster, Cluster);
                MAP(ResyncFileType_Map, Map);
                MAP(ResyncFileType_Unknown, Resync_Unkn);
                MAP(ResyncFileType_InternalErr, Resync_IntErr);
            default:
                BOOST_ASSERT(false);
                return Strings::ResyncFileType::Resync_Def;
            }
        }

#undef MAP
#define MAP(requestType, strObj) case requestType: return Strings::RequestType::strObj

        static const std::string& RequestTypeToString(RequestType requestType)
        {
            switch (requestType)
            {
                MAP(RequestType_Invalid, Req_Inv);
                MAP(RequestType_PutFile, PutFile);
                MAP(RequestType_ListFile, LsFile);
                MAP(RequestType_RenameFile, RenFile);
                MAP(RequestType_DeleteFile, DelFile);
                MAP(RequestType_GetFile, GetFile);
                MAP(RequestType_Login, Login);
                MAP(RequestType_CfsLogin, CfsLogin);
                MAP(RequestType_FxLogin, FxLogin);
                MAP(RequestType_CfsConnectBack, CfsConnBack);
                MAP(RequestType_CfsConnect, CfsConn);
                MAP(RequestType_Logout, Logout);
                MAP(RequestType_Heartbeat, Hbeat);
                MAP(RequestType_CfsHeartBeat, CfsHbeat);
            default:
                BOOST_ASSERT(false);
                return Strings::RequestType::Req_Def;
            }
        }

#undef MAP

#define MAP(requestFailureType, strObj) case requestFailureType: return Strings::ReqFailType::strObj

        static const std::string& RequestFailureToString(RequestFailure requestFailure)
        {
            switch (requestFailure)
            {
                MAP(RequestFailure_Success, Success);
                MAP(RequestFailure_MissingVer, MissingVer);
                MAP(RequestFailure_MissingId, MissingId);
                MAP(RequestFailure_MissingReqId, MissingReqId);
                MAP(RequestFailure_MissingHostId, MissingHostId);
                MAP(RequestFailure_MissingNonce, MissingNonce);
                MAP(RequestFailure_MissingFileName, MissingFileName);
                MAP(RequestFailure_MissingCfsSessionId, MissingCfsSessionId);
                MAP(RequestFailure_MissingCfsSecure, MissingCfsSecure);
                MAP(RequestFailure_MissingPathsTag, MissingPathsTag);
                MAP(RequestFailure_InvalidReqId, InvalidReqId);
                MAP(RequestFailure_UnknownRequest, UnknownRequest);
                MAP(RequestFailure_VerifLogin, VerifLogin);
                MAP(RequestFailure_VerifLogout, VerifLogout);
                MAP(RequestFailure_VerifGetFile, VerifGetFile);
                MAP(RequestFailure_VerifDelFile, VerifDelFile);
                MAP(RequestFailure_VerifPutFile, VerifPutFile);
                MAP(RequestFailure_VerifListFile, VerifListFile);
                MAP(RequestFailure_VerifRenFile, VerifRenFile);
                MAP(RequestFailure_VerifHeartBeat, VerifHeartBeat);
                MAP(RequestFailure_VerifCfsConnectBack, VerifCfsConnectBack);
                MAP(RequestFailure_VerifCfsConnect, VerifCfsConnect);
                MAP(RequestFailure_NotLoggedIn, NotLoggedIn);
                MAP(RequestFailure_BadCnonce, BadCnonce);
                MAP(RequestFailure_PutFileNoParam, PutFileNoParam);
                MAP(RequestFailure_PutFileNoParamVal, PutFileNoParamVal);
                MAP(RequestFailure_PutFileInvalidParams, PutFileInvalidParams);
                MAP(RequestFailure_PutFileParamsReadTimedOut, PutFileParamsReadTimedOut);
                MAP(RequestFailure_PutFileParamsFailed, PutFileParamsFailed);
                MAP(RequestFailure_PutFileMissingMoreData, PutFileMissingMoreData);
                MAP(RequestFailure_PutFileExpectedMoreData, PutFileExpectedMoreData);
                MAP(RequestFailure_NoCfsPort, NoCfsPort);
                MAP(RequestFailure_FileAccessDenied, FileAccessDenied);
                MAP(RequestFailure_TimedOut, TimedOut);
                MAP(RequestFailure_ErrorInResponse, ErrorInResponse);
                MAP(RequestFailure_CfsInsecureRequest, CfsInsecureRequest);
                MAP(RequestFailure_HandshakeFailed, HandshakeFailed);
                MAP(RequestFailure_HandshakeUnknownError, HandshakeUnknownError);
                MAP(RequestFailure_NwReadFailure, NwReadFailure);
                MAP(RequestFailure_HandleAsyncReadSomeUnknownError, HandleAsyncReadSomeUnknownError);
                MAP(RequestFailure_HandleAsyncPutFileUnknownError, HandleAsyncPutFileUnknownError);
                MAP(RequestFailure_ProcessRequestMoreDataExpected, ProcessRequestMoreDataExpected);
                MAP(RequestFailure_ProtocolHandlerError, ProtocolHandlerError);
                MAP(RequestFailure_ProtocolHandlerUnknownError, ProtocolHandlerUnknownError);
                MAP(RequestFailure_RequestHandlerProcessError, RequestHandlerProcessError);
                MAP(RequestFailure_PutFileInProgress, PutFileInProgress);
                MAP(RequestFailure_LoginFailed, LoginFailed);
                MAP(RequestFailure_LogoutFailed, LogoutFailed);
                MAP(RequestFailure_FileOpenFailed, FileOpenFailed);
                MAP(RequestFailure_FileBadState, FileBadState);
                MAP(RequestFailure_FileWriteFailed, FileWriteFailed);
                MAP(RequestFailure_FileReadFailed, FileReadFailed);
                MAP(RequestFailure_FileInvalidOffset, FileInvalidOffset);
                MAP(RequestFailure_NwWriteFailed, NwWriteFailed);
                MAP(RequestFailure_HandleAsyncGetFileUnknownError, HandleAsyncGetFileUnknownError);
                MAP(RequestFailure_CompleteCfsConnectFailed, CompleteCfsConnectFailed);
                MAP(RequestFailure_FileOpenInSession, FileOpenInSession);
                MAP(RequestFailure_DeleteFileUnknownError, DeleteFileUnknownError);
                MAP(RequestFailure_DeleteFileFailed, DeleteFileFailed);
                MAP(RequestFailure_RenameFileFailed, RenameFileFailed);
                MAP(RequestFailure_ListFileFailed, ListFileFailed);
                MAP(RequestFailure_HeartbeatFailed, HeartbeatFailed);
                MAP(RequestFailure_FileFlushFailed, FileFlushFailed);
                MAP(RequestFailure_UnknownError, UnknownError);
                MAP(RequestFailure_SessionDestroyed, SessionDestroyed);
                MAP(RequestFailure_ServerStopping, ServerStopping);
                MAP(RequestFailure_MissingOldName, MissingOldName);
                MAP(RequestFailure_MissingNewName, MissingNewName);
                MAP(RequestFailure_AcceptError, AcceptErr);
                MAP(RequestFailure_Deprecated_AsyncAcceptHandlingFailed, Deprecated_AsyncAccHandlFailed);
                MAP(RequestFailure_AcceptStartSessionFailed, AcceptStartSessionFailed);
                MAP(RequestFailure_AcceptNewSessionFailed, AcceptNewSessionFailed);
                MAP(RequestFailure_HandleReadEntireDataFromSocketAsyncUnknownError, HandleReadEntireDataFromSocketAsyncUnknownError);
                MAP(RequestFailure_CumulativeThrottlingFailure, CumulativeThrottlingFailure);
                MAP(RequestFailure_DiffThrottlingFailure, DiffThrottlingFailure);
                MAP(RequestFailure_ResyncThrottlingFailure, ResyncThrottlingFailure);
            default:
                BOOST_ASSERT(false);
                return Strings::ReqFailType::ReqFail_Def;
            }
        }

#undef MAP

#define MAP(telFailure, strObj) case telFailure: return Strings::TelemetryFailures::strObj

        static const std::string& TelemetryFailureToString(TelemetryFailure telFailure)
        {
            switch (telFailure)
            {
                MAP(TelFailure_None, None);
                MAP(TelFailure_Alloc, Alloc);
                MAP(TelFailure_AddToDiffTel, AddToDiffTel);
                MAP(TelFailure_AddToResyncTel, AddToResyncTel);
                MAP(TelFailure_AddToSrcGenTel, AddToSrcGenTel);
                MAP(TelFailure_AddToGlobalTel, AddToGlobalTel);
                MAP(TelFailure_DiffEmptyMeta, DiffEmptyMeta);
                MAP(TelFailure_ResyncEmptyMeta, ResyncEmptyMeta);
                MAP(TelFailure_AddTelUnknFType, AddTelUnknFType);
                MAP(TelFailure_SrcGenEmptyMeta, SrcGenEmptyMeta);
                MAP(TelFailure_AddTelReachedEof, AddTelReachedEof);
                MAP(TelFailure_AddToTelUnknErr, AddToTelUnknErr);
            default:
                BOOST_ASSERT(false);
                return Strings::TelemetryFailures::TelFail_Def;
            }
        }

#undef MAP

#define MAP(dataError, strObj) case dataError: return Strings::TelemetryDataError::strObj

        static const std::string& TelemetryDataErrorToString(RequestTelemetryDataError dataError)
        {
            switch (dataError)
            {
                MAP(ReqTelErr_None, None);
                MAP(ReqTelErr_TimerAlreadyStarted, TimerStarted);
                MAP(ReqTelErr_TimerNotStarted, TimerNStarted);
                MAP(ReqTelErr_AnotherOpInProgress, AnothOpInProg);
                MAP(ReqTelErr_UnexpectedOpInProgress, UnexpOpInProg);
                MAP(ReqTelErr_DanglingCurrOp, DangCurrOp);
                MAP(ReqTelErr_NumReqsLE0, NumReqsLE0);
            default:
                BOOST_ASSERT(false);
                return Strings::TelemetryDataError::TelDatErr_Def;
            }
        }

#undef MAP

    private:
        Stringer() {} // Truly static class
    };
}

#undef STR

#endif // !CXPS_TEL_STRINGER_H
