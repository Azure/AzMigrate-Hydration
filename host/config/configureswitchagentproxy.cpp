
//----------------------------------------------------------------------------------------------
//                        HEADERS
//----------------------------------------------------------------------------------------------
#include <boost/lexical_cast.hpp>
#include <ace/Mutex.h>
#include <ace/Guard_T.h>
#include "configureswitchagentproxy.h"
#include "switchinitialsettings.h"
#include "serializeswitchinitialsettings.h"


//----------------------------------------------------------------------------------------------
//                        CONSTANTS
//----------------------------------------------------------------------------------------------

static const char DELIMITER[] = "-";
static const char KEY_VT[] = "vt";
static const char KEY_VT_LUN[] = "vtlun";
static const char DISK_OID[] = "dk";
static const char VOLUME_OID[] = "vl";
static const char VT_OID[] = "vt";

static const char UPDATE_FR_OPERATION_STATE[] = "updateFrameRedirectOperationState";
static const char SEND_DISCOVERED_LUN_INFO[] = "sendDiscoveredLunInfo";
static const char SEND_EVENT_NOTIFY_INFO[] = "sendEventNotifyInfo";
static const char REPORT_SWITCH_AGENT_REBOOT[] = "reportSwitchAgentReboot";
static const char REPORT_SA_IO_ERROR_EXCEPTIONS[] = "reportSaIoErrorExceptions";
static const char REPORT_SA_APPLIANCE_LUN_EXCEPTIONS[] = "reportSaApplianceLunExceptions";
static const char REPORT_SA_PHYSICAL_LUN_EXCEPTIONS[] = "reportSaPhysicalLunExceptions";



bool ConfigureSwitchAgentProxy::updateFrameRedirectOperationState(FrameRedirectOperation& frameRedirectOperation)const
{
     try
	{
		std::string data = m_transport( marshalCxCall( m_dpc, UPDATE_FR_OPERATION_STATE, frameRedirectOperation));
		 return unmarshal<bool>( data );
	}
	catch (ContextualException const & ie) {
		DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendDiscoveredLunInfoToCx:%s", ie.what());             
        } catch (std::exception const & e) {
			DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendDiscoveredLunInfoToCx:%s", e.what());            
        } catch (...) {
			DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendDiscoveredLunInfoToCx: unknown exception\n");            
        }
		return false;
}

bool ConfigureSwitchAgentProxy::sendDiscoveredLunInfoToCx(PhysicalLunDiscoveryInfoList &pLunDiscList)const
{
     try
	{
		return unmarshal<bool>(m_transport( marshalCxCall( m_dpc, SEND_DISCOVERED_LUN_INFO, pLunDiscList)));
	 }
	catch (ContextualException const & ie) {
		DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendDiscoveredLunInfoToCx:%s", ie.what());             
        } catch (std::exception const & e) {
			DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendDiscoveredLunInfoToCx:%s", e.what());            
        } catch (...) {
			DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendDiscoveredLunInfoToCx: unknown exception\n");            
        }
		return false;
}

bool ConfigureSwitchAgentProxy::reportSwitchAgentReboot() const
{
    try
	{
		return unmarshal<bool>(m_transport( marshalCxCall (m_dpc, REPORT_SWITCH_AGENT_REBOOT)) );
	}
	catch (ContextualException const & ie) {
                DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::reportSwitchAgentReboot:%s", ie.what());
        } catch (std::exception const & e) {
                        DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::reportSwitchAgentReboot:%s", e.what());
        } catch (...) {
                        DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::reportSwitchAgentReboot: unknown exception\n");
        }

	DebugPrintf(SV_LOG_ERROR, " ConfigureSwitchAgentProxy::reportSwitchAgentReboot returning false ...\n"); 	
	return false;
	
}

bool ConfigureSwitchAgentProxy::getUseConfiguredHostname() const {
    return m_localConfigurator.getUseConfiguredHostname();
}

std::string ConfigureSwitchAgentProxy::getConfiguredHostname() const {
    return m_localConfigurator.getConfiguredHostname();
}

bool ConfigureSwitchAgentProxy::getUseConfiguredIpAddress() const {
    return m_localConfigurator.getUseConfiguredIpAddress();
}

std::string ConfigureSwitchAgentProxy::getConfiguredIpAddress() const {
    return m_localConfigurator.getConfiguredIpAddress();
}

std::string ConfigureSwitchAgentProxy::getInstallPath() const
{
	return m_localConfigurator.getInstallPath();
}

std::string ConfigureSwitchAgentProxy::getFabricWorldWideName() const
{
        return m_localConfigurator.getFabricWorldWideName();
}

string ConfigureSwitchAgentProxy::getSerializedPersistSwitchInitialSettings(PersistSwitchInitialSettings & pSwInitSettings)
{
        std::ostringstream s;
        s << cxArg( (const PersistSwitchInitialSettings &)pSwInitSettings);
	return s.str();
}

PersistSwitchInitialSettings  ConfigureSwitchAgentProxy::getUnserializedPersistSwitchInitialSettings(string serialstr)
{
       return unmarshal<PersistSwitchInitialSettings> (serialstr);
}

bool ConfigureSwitchAgentProxy::sendEventNotifyInfoSettingsToCx(EventNotifyInfoSettings eventNotifyInfoSettings)const
{
     try
	{
	  return unmarshal<bool> (m_transport( marshalCxCall( m_dpc, SEND_EVENT_NOTIFY_INFO, eventNotifyInfoSettings)));	
	}
	catch (ContextualException const & ie) {
		DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendEventNotifyInfoSettingsToCx:%s", ie.what());             
        } catch (std::exception const & e) {
			DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendEventNotifyInfoSettingsToCx:%s", e.what());            
        } catch (...) {
			DebugPrintf(SV_LOG_ERROR, "ConfigureSwitchAgentProxy::SendEventNotifyInfoSettingsToCx: unknown exception\n");            
        }
		return false;
}

bool ConfigureSwitchAgentProxy::reportApplianceLunEvent(SwitchAgentExceptionsList& applianceEventList)const {
    return unmarshal<bool>(m_transport( marshalCxCall( m_dpc,REPORT_SA_APPLIANCE_LUN_EXCEPTIONS,applianceEventList)));
}

bool ConfigureSwitchAgentProxy::reportPhysicalLunEvent(SwitchAgentExceptionsList& physicalEventList)const {
    return unmarshal<bool>(m_transport( marshalCxCall( m_dpc,REPORT_SA_PHYSICAL_LUN_EXCEPTIONS,physicalEventList)));
}

bool ConfigureSwitchAgentProxy::reportIoErrorException(SwitchAgentExceptionsList& ioErrorExceptionList)const {
    return unmarshal<bool>(m_transport( marshalCxCall( m_dpc,REPORT_SA_IO_ERROR_EXCEPTIONS,ioErrorExceptionList)));
}


