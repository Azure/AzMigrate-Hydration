#ifndef CXPS_REQUEST_TELEMETRY_DATA_H
#define CXPS_REQUEST_TELEMETRY_DATA_H

#include <string>

#include <boost/atomic.hpp>

#include "CxpsPerfCounters.h"

namespace CxpsTelemetry
{
    // Forward declarations
    class CxpsTelemetryLogger;
    class FileTelemetryData;
    class CxpsTelemetryRowBase;
    class RequestTelemetryData;
    void AddRequestDataToTelemetryLogger(const RequestTelemetryData&);

    class RequestTelemetryData
    {
        // TODO-SanKumar-1710: Make only the directly communicating member functions as a friend.
        friend class CxpsTelemetryLogger;
        friend class FileTelemetryData;
        friend class CxpsTelemetryRowBase;

    private:
        enum RequestOperation
        {
            RequestOp_None = 0,
            RequestOp_NwReadReqBlock,
            RequestOp_NwRead,
            RequestOp_NwReadDummyFromBuf,
            RequestOp_NwWrite,
            RequestOp_NwWriteDummyFromBuf,
            RequestOp_FRead,
            RequestOp_FWrite,
            RequestOp_FFlush,
            RequestOp_Compress,
            RequestOp_ReqSpecificOp
        };

        std::string m_sessionId;
        bool m_isUsingSsl;

        RequestTelemetryDataLevel m_dataLevel;
        // TODO-SanKumar-1711: Make it atomic and compare and update only if not already set.
        // Or should we go with flags?
        RequestTelemetryDataError m_dataError;
        RequestType m_requestType;
        RequestOperation m_currOperation;
        bool m_isDummyRW, m_isNewRequestBlockingRead;

        std::string m_hostId;
        std::string m_filePath;

        bool m_hasRespondedSuccess;
        bool m_isPutFileMoreData;
        boost::atomic<RequestFailure> m_requestFailure;

        boost_pt::ptime m_requestStartTime;
        boost_pt::ptime m_requestEndTime;
        SteadyClock::time_point m_requestStartTimePoint;
        SteadyClock::time_point m_requestEndTimePoint;
        int64_t m_numOfRequests;

        SteadyClock::time_point m_durationCalculationStart;

        chrono::nanoseconds m_newRequestBlockTime;
        chrono::nanoseconds m_totalReqSpecificOpTime;

        CxpsPerfCounters m_perfCounters;

        // returns throughput - time taken(mill seconds) to transfer 1 MB of data
        static double GetNormalizedThroughput(int64_t bytes, chrono::nanoseconds totalTimeTaken)
        {
            const int32_t _256KB = 256 * 1024;
            const int32_t _1MB = 1 * 1024 * 1024;

            double _256KBAlignedSizeBytes = (double)(bytes / _256KB + (bytes%_256KB == 0 ? 0 : 1)) * _256KB;
            double normalizedSizeInMB = _256KBAlignedSizeBytes / _1MB;
            double timeTakenInMilliSecs = (double)totalTimeTaken.count() / (boost::nano::den / boost::milli::den);

            return timeTakenInMilliSecs / normalizedSizeInMB; // Time(ms) taken per 1 MB
        }

        void StartInternalTimer(RequestOperation operation)
        {
            if (m_currOperation != RequestOp_None)
            {
                BOOST_ASSERT(false);
                if (m_dataError == ReqTelErr_None)
                    m_dataError = ReqTelErr_AnotherOpInProgress;
                m_currOperation = RequestOp_None; // reset
            }

            if (m_durationCalculationStart != Defaults::s_defaultTimePoint)
            {
                BOOST_ASSERT(false);
                if (m_dataError == ReqTelErr_None)
                    m_dataError = ReqTelErr_TimerAlreadyStarted;
                m_durationCalculationStart = Defaults::s_defaultTimePoint; // reset
            }

            m_currOperation = operation;
            m_durationCalculationStart = SteadyClock::now();
        }

        chrono::nanoseconds EndInternalTimer(RequestOperation operation)
        {
            SteadyClock::time_point startTime = m_durationCalculationStart;
            m_durationCalculationStart = Defaults::s_defaultTimePoint; // reset

            if (m_currOperation != operation)
            {
                BOOST_ASSERT(false);
                if (m_dataError == ReqTelErr_None)
                    m_dataError = ReqTelErr_UnexpectedOpInProgress;
                startTime = Defaults::s_defaultTimePoint;
            }
            m_currOperation = RequestOp_None; // reset

            if (startTime == Defaults::s_defaultTimePoint)
            {
                BOOST_ASSERT(false);
                if (m_dataError == ReqTelErr_None)
                    m_dataError = ReqTelErr_TimerNotStarted;
                return Defaults::s_zeroTime;
            }

            return SteadyClock::now() - startTime;
        }

        void RequestCompleted(bool internalSessionLogout)
        {
            BOOST_ASSERT(internalSessionLogout || // Any failure ended up logging out the session.
                m_isPutFileMoreData || // put file could've more data and so not completed.
                m_hasRespondedSuccess || // otherwise, we should've reached here only after server responded success to client.
                m_requestFailure == RequestFailure_ErrorInResponse); // unless there was an error in sending the response.

            if (m_requestFailure == RequestFailure_Success &&
                m_currOperation != RequestTelemetryData::RequestOp_None)
            {
                BOOST_ASSERT(false);
                m_dataError = ReqTelErr_DanglingCurrOp;
            }

            // Unless there has been a "successful" put request with "more data = true",
            // flush the collected data into telemetry.
            if (!(m_isPutFileMoreData && m_requestFailure == RequestFailure_Success))
            {
                if (m_dataLevel > ReqTelLevel_Server && m_numOfRequests <= 0)
                {
                    // The request count is started at the beginning of a request
                    // read and so there shouldn't be a case, where it's 0.

                    // TODO-SanKumar-1711: There's a known case, where this issue
                    // could arise. Currently, if the timeout thread runs in parallel
                    // with the processing/completion of the request, it would
                    // result in double posting of the request telemetry data,
                    // while the second thread posting it would be uploading an
                    // empty(just cleared) data. This should go away once the timeout
                    // thread is synchronized with the server thread of the session.
                    BOOST_ASSERT(false);
                    m_dataError = ReqTelErr_NumReqsLE0;
                }

                m_requestEndTimePoint = SteadyClock::now();
                m_requestEndTime = boost_pt::microsec_clock::universal_time();
                AddRequestDataToTelemetryLogger(*this);
                Clear();
            }
            // else
            // m_isPutFileMoreData == true.
            // Don't clear any member, since the file put will be continued with subsequent requests.
        }

    public:
        RequestTelemetryData()
            : m_dataLevel(ReqTelLevel_Server)
        {
            BOOST_STATIC_ASSERT(Tunables::FWriteLatencyBucketsCount >= 2); // At least one bounded and one unbounded bucket
            BOOST_ASSERT(Tunables::FWriteLatencyBuckets_ms[0] == 0);
            BOOST_STATIC_ASSERT(Tunables::NwReadLatencyBucketsCount >= 2); // At least one bounded and one unbounded bucket
            BOOST_ASSERT(Tunables::NwReadLatencyBuckets_ms[0] == 0);
            // TODO-SanKumar-1711: Write asserts to ensure the buckets are in increasing order

            m_hostId.reserve(37);
            //m_filePath.reserve(256 * sizeof(wchar_t));

            Clear();
        }

        ~RequestTelemetryData()
        {
            BOOST_ASSERT(IsEmpty());
        }

        void SetSessionProperties(const std::string &sessionId, bool isUsingSsl)
        {
            try
            {
                m_isUsingSsl = isUsingSsl;
                m_sessionId = sessionId;
            }
            GENERIC_CATCH_LOG_IGNORE("SetSessionProperties");
        }

        // Timer methods

        void StartingOp()
        {
            StartInternalTimer(RequestOp_ReqSpecificOp);
        }

        void CompletingOp()
        {
            m_totalReqSpecificOpTime += EndInternalTimer(RequestOp_ReqSpecificOp);
        }

        void StartingNwReadForNewRequest()
        {
            BOOST_ASSERT(m_isPutFileMoreData || IsEmpty());
            BOOST_ASSERT(!m_isDummyRW && !m_isNewRequestBlockingRead);

            // Although a request isn't received until the corresponding
            // read completion is received, we can't increment this counter at
            // the completion. Because the accounting wouldn't tally, if
            // there's a failure at this read. (i.e. it will be incorrect as 
            // num of req = 0, failed req = 1). So ++ing before receiving request.
            ++m_numOfRequests;

            m_isNewRequestBlockingRead = true;
            // StartingNwRead() will be called immediately after this.
        }

        void StartingDummyNwReadFromBuffer(int64_t expectedBytesRead)
        {
            BOOST_ASSERT(!m_isDummyRW && !m_isNewRequestBlockingRead);

            // TODO-SanKumar-1711: Assert the expected size at the completion service as double check.
            m_isDummyRW = true;
            StartingNwRead();
        }

        void StartingNwRead()
        {
            BOOST_ASSERT(!(m_isDummyRW && m_isNewRequestBlockingRead));
            BOOST_ASSERT(m_numOfRequests > 0);

            RequestOperation typeOfNwRead;
            if (m_isNewRequestBlockingRead)
                typeOfNwRead = RequestOp_NwReadReqBlock;
            else if (m_isDummyRW)
                typeOfNwRead = RequestOp_NwReadDummyFromBuf;
            else
                typeOfNwRead = RequestOp_NwRead;

            if (typeOfNwRead != RequestOp_NwReadDummyFromBuf)
                ++m_perfCounters.m_totalNwReadCnt;

            StartInternalTimer(typeOfNwRead);
        }

        void CompletingNwRead(int64_t bytesRead)
        {
            BOOST_ASSERT(!(m_isDummyRW && m_isNewRequestBlockingRead));
            BOOST_ASSERT(m_numOfRequests > 0 && m_perfCounters.m_totalNwReadCnt > 0);

            RequestOperation typeOfNwRead;
            if (m_isNewRequestBlockingRead)
                typeOfNwRead = RequestOp_NwReadReqBlock;
            else if (m_isDummyRW)
                typeOfNwRead = RequestOp_NwReadDummyFromBuf;
            else
                typeOfNwRead = RequestOp_NwRead;

            m_isNewRequestBlockingRead = false; // reset
            m_isDummyRW = false; // reset

            chrono::nanoseconds timeTaken = EndInternalTimer(typeOfNwRead);

            // RequestOp_NwReadDummyFromBuf doesn't have to update any stat, since
            // a NwRead operation before would've accounted for it.
            if (typeOfNwRead == RequestOp_NwReadDummyFromBuf)
                return;

            m_perfCounters.m_totalNwReadBytes += bytesRead;
            ++m_perfCounters.m_succNwReadCnt;

            if (typeOfNwRead == RequestOp_NwRead)
            {
                m_perfCounters.m_totalNwReadTime += NANO_TO_TICKS(timeTaken);

                Utils::IncrementBucket(
                    Tunables::NwReadLatencyBuckets_ms,
                    GetNormalizedThroughput(bytesRead, timeTaken),
                    m_perfCounters.m_nwReadBuckets,
                    Tunables::NwReadLatencyBucketsCount);
            }
            else // typeOfNwRead == RequestOp_NwReadReqBlock
            {
                if (!m_isPutFileMoreData)
                {
                    m_requestStartTime = boost_pt::microsec_clock::universal_time();
                    m_requestStartTimePoint = SteadyClock::now();
                    m_newRequestBlockTime = timeTaken;
                }
                else
                {
                    m_perfCounters.m_totalPutReqInterRequestTime += NANO_TO_TICKS(timeTaken);
                }
            }
        }

        void ReceivedNwEof()
        {
            // On receiving EOF, consider the read as NOOP. Most likely, this would
            // end in a protocol error.
            BOOST_ASSERT(m_currOperation == RequestOp_NwRead || m_currOperation == RequestOp_NwReadReqBlock);

            // TODO-SanKumar-1710: We are considering this as a network read cnt++.
            // The client has the right to send EOF anywhere within the timeout, so should be fine.
            CompletingNwRead(0);
        }

        void StartingDummyNwWriteFromBuffer(int64_t expectedBytesWritten)
        {
            BOOST_ASSERT(!m_isDummyRW && !m_isNewRequestBlockingRead);

            // TODO-SanKumar-1711: Assert the expected size at the completion service as double check.
            m_isDummyRW = true;
            StartingNwWrite();
        }

        void StartingNwWrite()
        {
            BOOST_ASSERT(!m_isNewRequestBlockingRead);

            RequestOperation typeOfNwWrite = m_isDummyRW ? RequestOp_NwWriteDummyFromBuf : RequestOp_NwWrite;

            if (typeOfNwWrite != RequestOp_NwWriteDummyFromBuf)
                ++m_perfCounters.m_totalNwWriteCnt;

            StartInternalTimer(typeOfNwWrite);
        }

        void CompletingNwWrite(int64_t bytesWritten)
        {
            BOOST_ASSERT(!m_isNewRequestBlockingRead);

            RequestOperation typeOfNwWrite = m_isDummyRW ? RequestOp_NwWriteDummyFromBuf : RequestOp_NwWrite;
            m_isDummyRW = false;

            chrono::nanoseconds timeTaken = EndInternalTimer(typeOfNwWrite);

            // RequestOp_NwWriteDummyFromBuf doesn't have to update any stat, since
            // a NwWrite operation before would've accounted for it.
            if (typeOfNwWrite == RequestOp_NwWriteDummyFromBuf)
                return;

            // typeOfNwWrite == RequestOp_NwWrite
            m_perfCounters.m_totalNwWriteTime += NANO_TO_TICKS(timeTaken);
            m_perfCounters.m_totalNwWrittenBytes += bytesWritten;
            ++m_perfCounters.m_succNwWriteCnt;
        }

        void StartingFileWrite()
        {
            ++m_perfCounters.m_totalFileWriteCnt;
            StartInternalTimer(RequestOp_FWrite);
        }

        void CompletingFileWrite(int64_t bytesWritten)
        {
            chrono::nanoseconds timeTaken = EndInternalTimer(RequestOp_FWrite);
            m_perfCounters.m_totalFileWriteTime += NANO_TO_TICKS(timeTaken);
            m_perfCounters.m_totalFileWrittenBytes += bytesWritten;

            Utils::IncrementBucket(
                Tunables::FWriteLatencyBuckets_ms,
                GetNormalizedThroughput(bytesWritten, timeTaken),
                m_perfCounters.m_fWriteBuckets,
                Tunables::FWriteLatencyBucketsCount);
            ++m_perfCounters.m_succFileWriteCnt;
        }

        void StartingFileRead()
        {
            ++m_perfCounters.m_totalFileReadCnt;
            StartInternalTimer(RequestOp_FRead);
        }

        void CompletingFileRead(int64_t bytesRead)
        {
            m_perfCounters.m_totalFileReadTime += NANO_TO_TICKS(EndInternalTimer(RequestOp_FRead));
            m_perfCounters.m_totalFileReadBytes += bytesRead;
            ++m_perfCounters.m_succFileReadCnt;
        }

        void StartingFileCompression()
        {
            StartInternalTimer(RequestOp_Compress);
        }

        void CompletingFileCompression()
        {
            m_perfCounters.m_totalFileCompressTime += NANO_TO_TICKS(EndInternalTimer(RequestOp_Compress));
        }

        void StartingFileFlush()
        {
            StartInternalTimer(RequestOp_FFlush);
        }

        void CompleteingFileFlush()
        {
            m_perfCounters.m_totalFileFlushTime += NANO_TO_TICKS(EndInternalTimer(RequestOp_FFlush));
        }

        // Getter methods

        bool HasRespondedSuccess()
        {
            return m_hasRespondedSuccess;
        }

        RequestFailure GetRequestFailure() const
        {
            return m_requestFailure.load(boost::memory_order_relaxed);
        }

        // Setter methods

        void MarkPutFileMoreData(bool moreData)
        {
            m_isPutFileMoreData = moreData;
        }

        void SuccessfullyResponded()
        {
            m_hasRespondedSuccess = true;
        }

        void SetRequestFailure(RequestFailure requestFailure)
        {
            // Update the failure, iff no error was already set. This is helpful
            // in ensuring the closest exception/error handler pops a specific
            // failure than the outer handler giving generic error.
            // This way original cause of the failure is preserved. Other examples
            // are request also timing out, response failing to send error, etc.
            RequestFailure allowedPrevFailure = RequestFailure_Success;

            // Ensuring atomicity of this exchange instead of using the costly
            // lock over all the members of this class. In a session, all the
            // other members are updated serially, but during timeout this is the
            // only member that could be set in parallel.
            if (m_requestFailure.compare_exchange_strong(allowedPrevFailure, requestFailure, boost::memory_order_seq_cst))
            {
                // If the error is set, while one of the following ops is in
                // progress, then increment the corresponding failed count.

                switch (m_currOperation)
                {
                case RequestOp_NwReadReqBlock:
                case RequestOp_NwRead:
                    ++m_perfCounters.m_failedNwReadCnt;
                    break;
                case RequestOp_NwWrite:
                    ++m_perfCounters.m_failedNwWriteCnt;
                    break;
                case RequestOp_FRead:
                    ++m_perfCounters.m_failedFileReadCnt;
                    break;
                case RequestOp_FWrite:
                    ++m_perfCounters.m_failedFileWriteCnt;
                    break;
                default: //NOOP
                    break;
                }

                // TODO-SanKumar-1711: Should we reset m_currOperation here?
                // Could that cause a telemetry data error wrongly?
            }
        }

        void RequestCompleted()
        {
            RequestCompleted(false); // Signalled externally (only on successful completion/list file not found error).
        }

        void AcquiredRequestType(RequestType requestType)
        {
            // TODO-SanKumar-1710: Should we also take the request id at this point
            // and assert/runtime-check that all the data updates are coming for
            // the same request id? Overkill?

            m_requestType = requestType;
        }

        // Level movers

        void AcquiredHostId(const std::string& hostId)
        {
            // Before subsequent request handling, the host id of the session is
            // repeatedly retried, making any failure in assigning at login is transient.
            BOOST_ASSERT(m_hostId.empty() || m_hostId == hostId);

            try
            {
                // This is attempted at the start of every request handling, so
                // averting updates in case of redundant retry.
                if (!hostId.empty() && m_hostId.empty())
                {
                    m_hostId = hostId;
                    m_dataLevel = ReqTelLevel_Session;
                }
            }
            GENERIC_CATCH_LOG_ACTION("AcquiredHostId", m_hostId.clear());
        }

        void AcquiredFilePath(const std::string& filePath)
        {
            try
            {
                m_filePath = filePath;
                m_dataLevel = ReqTelLevel_File;
            }
            GENERIC_CATCH_LOG_ACTION("AcquiredFilePath", m_filePath.clear());
        }

        void SessionLoggingOut()
        {
            // TODO-SanKumar-1710: If the error is PutFileInProgress, then the
            // request telemetry data object contains the merged statistics of
            // the prev put requests and the current request. But when we log an
            // error, the error would always correspond to the current request.
            // There should be another put telemetry failure along with its stats addded.

            // TODO-SanKumar-1710: Should we track number of sessions as well? (created, active, destroyed)

            // m_hasRespondedSuccess == true, in case of successful logout.
            if (!m_hasRespondedSuccess && GetRequestFailure() == RequestFailure_Success)
            {
                // The session is logging out due to an error, but telemetry logic
                // hasn't captured the specific error.
                SetRequestFailure(RequestFailure_UnknownError);
            }

            RequestCompleted(true);
        }

        void Clear()
        {
            // m_sessionId, m_isUsingSsl remain the same for the life time of the session. Don't reset.

            m_dataError = ReqTelErr_None;
            m_requestType = RequestType_Invalid;
            m_currOperation = RequestOp_None;
            m_isDummyRW = false;
            m_isNewRequestBlockingRead = false;

            // m_dataLevel, m_hostId remains through out the session
            if (m_dataLevel == ReqTelLevel_File)
            {
                // The file path belonged to the particular request. Step back
                // to receive the file path from the next request.
                m_dataLevel = ReqTelLevel_Session;
            }
            m_filePath.clear();

            m_hasRespondedSuccess = false;
            m_isPutFileMoreData = false;
            m_requestFailure = RequestFailure_Success; // 0

            m_requestStartTime = Defaults::s_defaultPTime;
            m_requestEndTime = Defaults::s_defaultPTime;
            m_requestStartTimePoint = Defaults::s_defaultTimePoint;
            m_requestEndTimePoint = Defaults::s_defaultTimePoint;
            m_numOfRequests = 0;

            m_durationCalculationStart = Defaults::s_defaultTimePoint;
            m_newRequestBlockTime = Defaults::s_zeroTime;
            m_totalReqSpecificOpTime = Defaults::s_zeroTime;

            m_perfCounters.Clear();
        }

        bool IsEmpty()
        {
            return
                m_dataError == ReqTelErr_None &&
                m_requestType == RequestType_Invalid &&
                m_currOperation == RequestOp_None &&
                m_isDummyRW == false &&
                m_isNewRequestBlockingRead == false &&
                m_filePath.empty() &&
                m_hasRespondedSuccess == false &&
                m_isPutFileMoreData == false &&
                m_requestFailure == RequestFailure_Success &&
                m_requestStartTime == Defaults::s_defaultPTime &&
                m_requestEndTime == Defaults::s_defaultPTime &&
                m_requestStartTimePoint == Defaults::s_defaultTimePoint &&
                m_requestEndTimePoint == Defaults::s_defaultTimePoint &&
                m_numOfRequests == 0 &&
                // TODO-SanKumar-1711: TBD
                // memcmp(m_nwReadBuckets) == 0
                // memcmp(m_fWriteBuckets) == 0
                m_durationCalculationStart == Defaults::s_defaultTimePoint &&
                m_newRequestBlockTime == Defaults::s_zeroTime &&
                m_totalReqSpecificOpTime == Defaults::s_zeroTime &&
                m_perfCounters.IsEmpty();
        }
    };
}

#endif // !CXPS_REQUEST_TELEMETRY_DATA_H
