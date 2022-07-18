#ifndef GLOBAL_TEL_ROW_H
#define GLOBAL_TEL_ROW_H

#include "CxpsTelemetryRowBase.h"
#include "inmsafecapis.h"

namespace CxpsTelemetry
{
    class GlobalCxpsTelemetryRow : public CxpsTelemetryRowBase
    {
    public:
        GlobalCxpsTelemetryRow()
            : CxpsTelemetryRowBase(MsgType_CxpsGlobal)
        {
            ClearSelfMembers();
        }

        GlobalCxpsTelemetryRow(boost_pt::ptime loggerStartTime)
            : CxpsTelemetryRowBase(MsgType_CxpsGlobal, loggerStartTime, std::string(), std::string())
        {
            ClearSelfMembers();
        }

        virtual ~GlobalCxpsTelemetryRow()
        {
            BOOST_ASSERT(IsCacheObj() || IsEmpty());
        }

        void ReportMessageDrop(MessageType msgType)
        {
            RecursiveLockGuard guard(m_rowObjLock);

            // TODO-SanKumar-1710: TBD
            //UpdateRequestInfo();

            try
            {
                ++m_totalMessageDropCnt;
                ++m_messageDropCnt[msgType];
            }
            GENERIC_CATCH_LOG_IGNORE("GlobalTelemetry::ReportMessageDrop");
        }

        void ReportRequestTelemetryDataError(RequestTelemetryDataError dataError)
        {
            RecursiveLockGuard guard(m_rowObjLock);

            // TODO-SanKumar-1710: TBD
            //UpdateRequestInfo();

            try
            {
                ++m_totalDataErrors;
                ++m_telDataErrors[dataError];
            }
            GENERIC_CATCH_LOG_IGNORE("GlobalTelemetry::ReportRequestTelemetryDataError");
        }

        void ReportTelemetryFailure(TelemetryFailure telFailure)
        {
            RecursiveLockGuard guard(m_rowObjLock);

            // TODO-SanKumar-1710: TBD
            //UpdateRequestInfo();

            try
            {
                ++m_totalTelFailures;
                ++m_telFailures[telFailure];
            }
            GENERIC_CATCH_LOG_IGNORE("GlobalTelemetry::ReportTelemetryFailure");
        }

        void ReportRequestFailure(RequestFailure reqFailure)
        {
            RecursiveLockGuard guard(m_rowObjLock);

            // TODO-SanKumar-1710: TBD
            //UpdateRequestInfo();

            try
            {
                ++m_failureMap[reqFailure];
            }
            GENERIC_CATCH_LOG_IGNORE("GlobalTelemetry::ReportRequestFailure");
            // TODO-SanKumar-1711: Add MsgDrop for this exception
        }

        bool AddToTelemetry(const RequestTelemetryData &reqTelData)
        {
            RecursiveLockGuard guard(m_rowObjLock);

            // TODO-SanKumar-1710:
            //UpdateRequestInfo();

            try
            {
                ++m_failureMap[reqTelData.GetRequestFailure()];
            }
            GENERIC_CATCH_LOG_ACTION("GlobalTelemetry::AddToTelemetry", return false);

            return true;
        }

        virtual bool IsEmpty() const
        {
            RecursiveLockGuard guard(m_rowObjLock);

            return
                m_totalMessageDropCnt == 0 &&
                m_totalDataErrors == 0 &&
                m_totalTelFailures == 0 &&
                m_failureMap.empty();
        }

        // TODO-SanKumar-1711: Make this non-copyable and do for all in the cxps telemetry.
        class CustomData
        {
        public:
            CustomData(const GlobalCxpsTelemetryRow &rowObj)
                : m_rowObj(rowObj)
            {
            }

#define TO_MDS(string, element) JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::GlobalTel::string, element)
            void serialize(JSON::Adapter &adapter)
            {
                JSON::Class root(adapter, "CustomData", false);

                Utils::NameValueCounters<int64_t> countersObj;
                if (countersObj.Load<MessageType>(m_rowObj.m_messageDropCnt, Stringer::MessageTypeToString))
                    TO_MDS(MsgDrops, countersObj);

                if (countersObj.Load<RequestFailure>(m_rowObj.m_failureMap, Stringer::RequestFailureToString))
                    TO_MDS(ReqFailures, countersObj);

                if (countersObj.Load<TelemetryFailure>(m_rowObj.m_telFailures, Stringer::TelemetryFailureToString))
                    TO_MDS(TelFailures, countersObj);

                if (countersObj.Load<RequestTelemetryDataError>(m_rowObj.m_telDataErrors, Stringer::TelemetryDataErrorToString))
                    TO_MDS(TelDataErr, countersObj);

                JSON_T_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::Pid, s_psProcessId);
            }

#undef TO_MDS

        private:
            const GlobalCxpsTelemetryRow &m_rowObj;
        };

    protected:
        virtual void OnClear()
        {
            ClearSelfMembers();
        }

        virtual bool OnGetPrevWindow_CopyPhase1(CxpsTelemetryRowBase &prevWindowRow) const
        {
            GlobalCxpsTelemetryRow& prev = dynamic_cast<GlobalCxpsTelemetryRow&>(prevWindowRow);

            try
            {
                prev.m_totalMessageDropCnt = m_totalMessageDropCnt;
                inm_memcpy_s(prev.m_messageDropCnt, sizeof(prev.m_messageDropCnt), m_messageDropCnt, sizeof(m_messageDropCnt));
                prev.m_failureMap = m_failureMap;
                prev.m_totalTelFailures = m_totalTelFailures;
                inm_memcpy_s(prev.m_telFailures, sizeof(prev.m_telFailures), m_telFailures, sizeof(m_telFailures));
                prev.m_totalDataErrors = m_totalDataErrors;
                inm_memcpy_s(prev.m_telDataErrors, sizeof(prev.m_telDataErrors), m_telDataErrors, sizeof(m_telDataErrors));

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("GlobalCxpsTelemetryRow::OnGetPrevWindow_CopyPhase1", return false);
        }

        virtual bool OnGetPrevWindow_CopyPhase2(const CxpsTelemetryRowBase &prevWindowRow)
        {
            const GlobalCxpsTelemetryRow& prev = dynamic_cast<const GlobalCxpsTelemetryRow&>(prevWindowRow);

            try
            {
                ClearSelfMembers();

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("GlobalCxpsTelemetryRow::OnGetPrevWindow_CopyPhase2", return false);
        }

        virtual bool OnAddBackPrevWindow(const CxpsTelemetryRowBase &prevWindowRow)
        {
            const GlobalCxpsTelemetryRow& prev = dynamic_cast<const GlobalCxpsTelemetryRow&>(prevWindowRow);

            // Note that we should literally add it back (not memcpy), since there
            // could be new values updated into the telemetry, since the getPrev() call.
            try
            {
                m_totalMessageDropCnt += prev.m_totalMessageDropCnt;

                for (size_t ind = 0; ind < MsgType_Count; ind++)
                    m_messageDropCnt[ind] += prev.m_messageDropCnt[ind];

                for (FailureMap::const_iterator itr = prev.m_failureMap.begin(); itr != prev.m_failureMap.end(); itr++)
                    m_failureMap[itr->first] += itr->second; // If not present, an entry with 0 value would be created.

                m_totalTelFailures += prev.m_totalTelFailures;

                for (size_t ind = 0; ind < TelFailure_Count; ind++)
                    m_telFailures[ind] += prev.m_telFailures[ind];

                m_totalDataErrors += prev.m_totalDataErrors;

                for (size_t ind = 0; ind < ReqTelErr_Count; ind++)
                    m_telDataErrors[ind] += prev.m_telDataErrors[ind];

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("GlobalCxpsTelemetryRow::OnGetPrevWindow_CopyPhase2", return false);
        }

        virtual void RetrieveDynamicJsonData(std::string &dynamicJsonData)
        {
            CustomData customData(*this);
            dynamicJsonData = JSON::producer<CustomData>::convert(customData);
        }

    private:
        int64_t m_totalMessageDropCnt;
        int64_t m_messageDropCnt[MsgType_Count];
        FailureMap m_failureMap;
        int64_t m_totalTelFailures;
        int64_t m_telFailures[TelFailure_Count];
        int64_t m_totalDataErrors;
        int64_t m_telDataErrors[ReqTelErr_Count];

        void ClearSelfMembers()
        {
            m_totalMessageDropCnt = 0;
            memset(m_messageDropCnt, 0, sizeof(m_messageDropCnt));
            m_failureMap.clear();
            m_totalTelFailures = 0;
            memset(m_telFailures, 0, sizeof(m_telFailures));
            m_totalDataErrors = 0;
            memset(m_telDataErrors, 0, sizeof(m_telDataErrors));
        }
    };

    typedef boost::shared_ptr<GlobalCxpsTelemetryRow> GlobalCxpsTelemetryRowPtr;
}

#endif // !GLOBAL_TEL_ROW_H
