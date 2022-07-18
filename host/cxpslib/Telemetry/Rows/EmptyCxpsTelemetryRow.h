#ifndef EMPTY_CXPS_ROW_H
#define EMPTY_CXPS_ROW_H

#include "CxpsTelemetryRowBase.h"

namespace CxpsTelemetry
{
    class EmptyCxpsTelemetryRow : public CxpsTelemetryRowBase
    {
    public:
        EmptyCxpsTelemetryRow()
            : CxpsTelemetryRowBase(MsgType_Empty)
        {
        }

        EmptyCxpsTelemetryRow(boost_pt::ptime loggerStartTime)
            : CxpsTelemetryRowBase(MsgType_Empty, loggerStartTime, std::string(), std::string())
        {
        }

        virtual ~EmptyCxpsTelemetryRow()
        {
            BOOST_ASSERT(IsCacheObj() || IsEmpty());
        }

        virtual bool IsEmpty() const
        {
            return true; // Is always empty, as it doesn't have any members.
        }

        class CustomData
        {
        public:
            void serialize(JSON::Adapter &adapter)
            {
                JSON::Class root(adapter, "CustomData", false);

                JSON_T_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::Pid, s_psProcessId);
            }
        };

    protected:
        virtual void OnClear() {}

        virtual bool OnGetPrevWindow_CopyPhase1(CxpsTelemetryRowBase &prevWindowRow) const { return true; }

        virtual bool OnGetPrevWindow_CopyPhase2(const CxpsTelemetryRowBase &prevWindowRow) { return true; }

        virtual bool OnAddBackPrevWindow(const CxpsTelemetryRowBase &prevWindowRow) { return true; }

        virtual void RetrieveDynamicJsonData(std::string &dynamicJsonData)
        {
            CustomData customData;
            dynamicJsonData = JSON::producer<CustomData>::convert(customData);
        }
    };
}

#endif // !EMPTY_CXPS_ROW_H
