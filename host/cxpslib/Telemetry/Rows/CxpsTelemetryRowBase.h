#ifndef CXPS_TEL_ROW_BASE
#define CXPS_TEL_ROW_BASE

#include "../FileTelemetryData.h"
#include "../SourceFilePathParser.h"

namespace CxpsTelemetry
{
    typedef boost::lock_guard<boost::recursive_mutex> RecursiveLockGuard;
    typedef boost::lock_guard<boost::mutex> LockGuard;

    class ExpirableObject
    {
    public:
        // TODO-SanKumar-1710: Static assert expiry against the telemety logger polling time.
        static const int64_t ExpiryTimeNs = 60 * 60 * boost::nano::den; // 1 day

        ExpirableObject()
        {
            SignalObjectUsage();
        }

        void SignalObjectUsage()
        {
            SteadyClock::rep currTimeNs = SteadyClock::now().time_since_epoch().count();
            m_lastUsedTime = currTimeNs;
        }

        bool HasExpired()
        {
            return (SteadyClock::now().time_since_epoch().count() - m_lastUsedTime) >= ExpiryTimeNs;
        }

    private:
        boost::atomic<SteadyClock::rep> m_lastUsedTime;
    };

    class CxpsTelemetryRowBase : public ExpirableObject
    {
    protected:
        // Static member accessible to the sub-classes to fill in their CustomJson.
        static uint64_t s_psProcessId;

        // Recursive mutex used by this and the sub classes.
        mutable boost::recursive_mutex m_rowObjLock;

        CxpsTelemetryRowBase(MessageType messageType);
        CxpsTelemetryRowBase(MessageType messageType, boost_pt::ptime loggerStartTime, const std::string &hostId, const std::string &device);

        virtual void OnClear() = 0;

        // Phase 1 - copy previous window data from obj.
        virtual bool OnGetPrevWindow_CopyPhase1(CxpsTelemetryRowBase &prevWindowRow) const = 0;

        // Phase 2 - clear copied data in obj.
        virtual bool OnGetPrevWindow_CopyPhase2(const CxpsTelemetryRowBase &prevWindowRow) = 0;

        virtual bool OnAddBackPrevWindow(const CxpsTelemetryRowBase &prevWindowRow) = 0;

        // Generate the dynamic data to be logged into MDS
        virtual void RetrieveDynamicJsonData(std::string &dynamicJsonData) = 0;

        void MarkObjForModification()
        {
            m_isValid.exchange(false, boost::memory_order_acquire);
        }

        void MarkModCompletionForObj()
        {
            m_isValid.exchange(true, boost::memory_order_release);
        }

        bool IsObjValidSlim() const // Slim => no lock
        {
            return m_isValid.load(boost::memory_order_relaxed);
        }

        bool IsCacheObj() const
        {
            return !m_isOwned;
        }

    public:
        virtual ~CxpsTelemetryRowBase()
        {
            // TODO-SanKumar-1710: Can't do this check in destructor, since
            // IsEmpty() is a pure virtual function, while this call would act
            // like a regular local call instead of a virtual function (since at
            // destructor). So, implement an IsEmpty() check for this class too.
            // BOOST_ASSERT(!m_isOwned || IsEmpty());
        }

        // Clear the object
        void Clear();

        // Is the object empty?
        virtual bool IsEmpty() const = 0;

        // Cut and return till the data till current time, while continuing to collect future data.
        // The object of this class may not be valid after failure.
        bool GetPrevWindowRow(CxpsTelemetryRowBase &prevWindowRow, boost_pt::ptime timeOfRow);

        // On any failure, this method is used to add back the data cut for the previous window.
        // The object of this class may not be valid after failure.
        bool AddBackPrevWindow(const CxpsTelemetryRowBase &prevWindowRow);

        bool IsInvalidOrEmpty();
        bool IsValid();

        bool AddToTelemetry(const std::string &pendingReqInfo, const RequestTelemetryData &reqTelData);

        void SetSeqNumber(int64_t seqNum);

        MessageType GetMessageType();

        void serialize(JSON::Adapter &adapter);

        static void RefreshSystemIdentifiers();

    private:
        MessageType m_messageType;
        std::string m_pendingReqInfo;
        std::string m_hostId;
        std::string m_device;

        boost_pt::ptime m_loggerStartTime;
        boost_pt::ptime m_loggerEndTime;
        boost_pt::ptime m_reqsStartTime;
        boost_pt::ptime m_reqsEndTime;

        uint64_t m_seqNum;

        bool m_isOwned;
        // Making this atomic to utilize memory barrier through acquire and release semantics.
        boost::atomic<bool> m_isValid;

        // Static data members uniquely identifying the environment
        static std::string s_biosId, s_fabType;
        static std::string s_psHostId, s_psAgentVer;

        void ClearSelfMembers();
    };

    typedef boost::shared_ptr<CxpsTelemetryRowBase> CxpsTelemetryRowBasePtr;
}

#endif // !CXPS_TEL_ROW_BASE
