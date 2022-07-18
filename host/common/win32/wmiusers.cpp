#include <windows.h>
#include <atlbase.h>
#include <WinIoCtl.h>
#include "wmiusers.h"
#include "logger.h"
#include "portablehelpers.h"
#include "configwrapper.h"
#include "portablehelpersmajor.h"
#include "hostagenthelpers.h"

const char MACADDRESSCOLUMNNAME[] = "MACAddress";
const char DHCPENABLEDCOLUMNNAME[] = "DHCPEnabled";
const char IPADDRESSCOLUMNNAME[] = "IPAddress";
const char IPSUBNETCOLUMNNAME[] = "IPSubnet";
const char DEFAULTIPGATEWAYCOLUMNNAME[] = "DefaultIPGateway";
const char INDEXCOLUMNNAME[] = "Index";
const char DNSSERVERSEARCHORDERCOLUMNNAME[] = "DNSServerSearchOrder";
const char DNSHOSTNAMECOLUMNNAME[] = "DNSHostName";
const char NAMECOLUMNNAME[] = "Name";
const char MANUFACTURERCOLUMNNAME[] = "Manufacturer";
const char MAXSPEEDCOLUMNNAME[] = "MaxSpeed";
const char MODELCOLUMNNAME[] = "Model";

void CopyFromBstrSafeArrayToOutParams(VARIANT *pvt, std::string &valueString, const char &delim,
  const bool fillVector = false, std::vector<std::string>& valuesVector = std::vector<std::string>());
void UpdateDiskType(const std::string &diskname, Object *pdiskobj);

bool WmiComputerSystemRecordProcessor::Process(IWbemClassObject *precordobj)
{
    if (0 == m_pWmiComputerSystemClass)
    {
        m_ErrMsg = "wmi computer system class to fill is not supplied in its record processor";
        return false;
    }

    if (0 == precordobj)
    {
        m_ErrMsg = "record object is not specified for wmi computer system class";
        return false;
    }

    bool bprocessed = true;
    USES_CONVERSION;
    /* copy model */
    VARIANT vtProp;
    HRESULT hrCol;

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(MODELCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (V_VT(&vtProp) == VT_BSTR)
        {
            m_pWmiComputerSystemClass->model = W2A(vtProp.bstrVal);
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += MODELCOLUMNNAME;
        m_ErrMsg += " from computer system wmi class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(MANUFACTURERCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (V_VT(&vtProp) == VT_BSTR)
        {
            m_pWmiComputerSystemClass->manufacturer = W2A(vtProp.bstrVal);
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += MANUFACTURERCOLUMNNAME;
        m_ErrMsg += " from computer system wmi class";
    }

    return bprocessed;
}

void WmiComputerSystemRecordProcessor::WmiComputerSystemClass::Print(void)
{
    DebugPrintf(SV_LOG_DEBUG, "model: %s\n", model.c_str());
    DebugPrintf(SV_LOG_DEBUG, "manufacturer: %s\n", manufacturer.c_str());
}


bool WmiNetworkAdapterRecordProcessor::Process(IWbemClassObject *precordobj)
{
    if (0 == m_pNicInfos)
    {
        m_ErrMsg = "nic infos to fill is not supplied in process wmi network adapter class";
        return false;
    }

    if (0 == precordobj)
    {
        m_ErrMsg = "record object is not specified for wmi network adapter class";
        return false;
    }

    bool bprocessed = true;
    USES_CONVERSION;
    VARIANT vtProp;
    HRESULT hrCol;
    std::stringstream ssmessage;

    std::string hardwareaddress;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(MACADDRESSCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            hardwareaddress = W2A(V_BSTR(&vtProp));
        }
        VariantClear(&vtProp);

        ssmessage << MACADDRESSCOLUMNNAME << " : " << hardwareaddress << std::endl;
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += MACADDRESSCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter class";
        return bprocessed;
    }

    unsigned int index = ~0;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(INDEXCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_I4 == V_VT(&vtProp))
        {
            index = V_I4(&vtProp);
        }
        VariantClear(&vtProp);

        ssmessage << INDEXCOLUMNNAME << " : " << index << std::endl;
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += INDEXCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter class";
        return bprocessed;
    }

    if (~0 == index)
    {
        bprocessed = false;
        m_ErrMsg = "failed to get correct type for ";
        m_ErrMsg += INDEXCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter class";
        return bprocessed;
    }

    if (hardwareaddress.empty())
    {
        ssmessage << "Skipping record as " << MACADDRESSCOLUMNNAME << " is empty.";
        m_TraceMsg = ssmessage.str();
        return bprocessed;
    }

    Objects_t *pnicobjs;
    NicInfosIter_t nicinfoiter = m_pNicInfos->find(hardwareaddress);
    if (nicinfoiter != m_pNicInfos->end())
    {
        ssmessage << "Found existing NicInfo object for MAC address " << hardwareaddress << std::endl;
        Objects_t &nicobjs = nicinfoiter->second;
        pnicobjs = &nicobjs;
    }
    else
    {
        ssmessage << "Creating new NicInfo object for MAC address " << hardwareaddress << std::endl;
        std::pair<NicInfosIter_t, bool> pr = m_pNicInfos->insert(std::make_pair(hardwareaddress, Objects_t()));
        Objects_t &nicobjs = pr.first->second;
        pnicobjs = &nicobjs;
    }

    std::stringstream ssindex;
    ssindex << index;
    Object *p;
    ObjectsIter_t nicobjiter = std::find_if(pnicobjs->begin(), pnicobjs->end(), ObjectEqAttr(NSNicInfo::INDEX, ssindex.str()));
    if (nicobjiter != pnicobjs->end())
    {
        ssmessage << "Found existing NicInfo object for index " << index << std::endl;
        Object &nicobj = *nicobjiter;
        p = &nicobj;
    }
    else
    {
        ssmessage << "Creating new NicInfo object for index " << index << std::endl;
        ObjectsIter_t nit = pnicobjs->insert(pnicobjs->end(), Object());
        Object &nicobj = *nit;
        p = &nicobj;
        p->m_Attributes.insert(std::make_pair(NSNicInfo::INDEX, ssindex.str()));
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(NAMECOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            p->m_Attributes.insert(std::make_pair(NSNicInfo::NAME, W2A(V_BSTR(&vtProp))));
            ssmessage << NAMECOLUMNNAME << " : " << W2A(V_BSTR(&vtProp)) << std::endl;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += NAMECOLUMNNAME;
        m_ErrMsg += " from wmi network adapter class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(MANUFACTURERCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            p->m_Attributes.insert(std::make_pair(NSNicInfo::MANUFACTURER, W2A(V_BSTR(&vtProp))));
            ssmessage << MANUFACTURERCOLUMNNAME << " : " << W2A(V_BSTR(&vtProp)) << std::endl;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += MANUFACTURERCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(MAXSPEEDCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        /* TODO: this is not supported as of now;
         * but will this occur as I8 ? */
        if (VT_I8 == V_VT(&vtProp))
        {
            std::stringstream ssmaxspeed;
            ssmaxspeed << V_I8(&vtProp);
            p->m_Attributes.insert(std::make_pair(NSNicInfo::MAX_SPEED, ssmaxspeed.str()));
            ssmessage << MAXSPEEDCOLUMNNAME << " : " << ssmaxspeed.str() << std::endl;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += MAXSPEEDCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter class";
    }

    m_TraceMsg = ssmessage.str();

    return bprocessed;
}


bool WmiNetworkAdapterConfRecordProcessor::Process(IWbemClassObject *precordobj)
{
    if (0 == m_pNicInfos)
    {
        m_ErrMsg = "nic infos to fill is not supplied in process wmi network adapter configuration class";
        return false;
    }

    if (0 == precordobj)
    {
        m_ErrMsg = "record object is not specified for wmi network adapter configuration class";
        return false;
    }

    bool bprocessed = true;
    USES_CONVERSION;
    VARIANT vtProp;
    HRESULT hrCol;
    std::stringstream ssmessage;

    std::string hardwareaddress;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(MACADDRESSCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            hardwareaddress = W2A(V_BSTR(&vtProp));
        }
        VariantClear(&vtProp);

        ssmessage << MACADDRESSCOLUMNNAME << " : " << hardwareaddress << std::endl;
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += MACADDRESSCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
        return bprocessed;
    }
    
    unsigned int index = ~0;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(INDEXCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_I4 == V_VT(&vtProp))
        {
            index = V_I4(&vtProp);
        }
        VariantClear(&vtProp);
        ssmessage << INDEXCOLUMNNAME << " : " << index << std::endl;
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += INDEXCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
        return bprocessed;
    }

    if (~0 == index)
    {
        bprocessed = false;
        m_ErrMsg = "failed to get correct type for ";
        m_ErrMsg += INDEXCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
        return bprocessed;
    }

    if (hardwareaddress.empty())
    {
        ssmessage << "Skipping record as " << MACADDRESSCOLUMNNAME << " is empty.";
        m_TraceMsg = ssmessage.str();
        return bprocessed;
    }

    Objects_t *pnicobjs;
    NicInfosIter_t nicinfoiter = m_pNicInfos->find(hardwareaddress);
    if (nicinfoiter != m_pNicInfos->end())
    {
        ssmessage << "Found existing NicInfo object for MAC address " << hardwareaddress << std::endl;
        Objects_t &nicobjs = nicinfoiter->second;
        pnicobjs = &nicobjs;
    }
    else
    {
        ssmessage << "Creating new NicInfo object for MAC address " << hardwareaddress << std::endl;
        std::pair<NicInfosIter_t, bool> pr = m_pNicInfos->insert(std::make_pair(hardwareaddress, Objects_t()));
        Objects_t &nicobjs = pr.first->second;
        pnicobjs = &nicobjs;
    }

    std::stringstream ssindex;
    ssindex << index;
    Object *p;
    ObjectsIter_t nicobjiter = std::find_if(pnicobjs->begin(), pnicobjs->end(), ObjectEqAttr(NSNicInfo::INDEX, ssindex.str()));
    if (nicobjiter != pnicobjs->end())
    {
        ssmessage << "Found existing NicInfo object for index " << index << std::endl;
        Object &nicobj = *nicobjiter;
        p = &nicobj;
    }
    else
    {
        ssmessage << "Creating new NicInfo object for index " << index << std::endl;
        ObjectsIter_t nit = pnicobjs->insert(pnicobjs->end(), Object());
        Object &nicobj = *nit;
        p = &nicobj;
        p->m_Attributes.insert(std::make_pair(NSNicInfo::INDEX, ssindex.str()));
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(DHCPENABLEDCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BOOL == V_VT(&vtProp))
        {
            p->m_Attributes.insert(std::make_pair(NSNicInfo::IS_DHCP_ENABLED, STRBOOL(V_BOOL(&vtProp))));
            ssmessage << DHCPENABLEDCOLUMNNAME << " : " << STRBOOL(V_BOOL(&vtProp)) << std::endl;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += DHCPENABLEDCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(IPADDRESSCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        std::pair<AttributesIter_t, bool> pr = p->m_Attributes.insert(std::make_pair(NSNicInfo::IP_ADDRESSES, std::string()));
        AttributesIter_t nit = pr.first;
        std::vector<std::string> ipVector;
        CopyFromBstrSafeArrayToOutParams(&vtProp, nit->second, NSNicInfo::DELIM, true, ipVector);
        VariantClear(&vtProp);

        ssmessage << IPADDRESSCOLUMNNAME << " : ";
        std::vector<std::string>::iterator ipvecIter = ipVector.begin();
        for (/**/; ipvecIter != ipVector.end(); ipvecIter++)
            ssmessage << *ipvecIter << ", ";
        ssmessage << std::endl;

        /* insert ip_types attribute for all the ips */
        do
        {
            std::vector<std::string> physicalIPs;
            LONG result = GetAllPhysicalIPsOfSystem(physicalIPs);
            if (result)
            {
                DebugPrintf(SV_LOG_ERROR, "Failed to fetch physical ips, not registering ip_types attribute in NicInfo.\n");
                break;
            }

            ssmessage << "physicalIPs" << " : ";
            std::vector<std::string>::iterator pipvecIter = physicalIPs.begin();
            for (/**/; pipvecIter != physicalIPs.end(); pipvecIter++)
                ssmessage << *pipvecIter << ", ";
            ssmessage << std::endl;

            /* physicalIPs has physical IPs; mark them as such and rest as 'unknown'*/
            pr = p->m_Attributes.insert(std::make_pair(NSNicInfo::IP_TYPES, std::string()));
            nit = pr.first;
            for (int i = 0; i < ipVector.size(); ++i)
            {
                if (std::find(physicalIPs.begin(), physicalIPs.end(), ipVector[i]) != physicalIPs.end())
                {
                    nit->second += NSNicInfo::IP_TYPE_PHYSICAL;
                }
                else
                {
                    nit->second += NSNicInfo::IP_TYPE_UNKNOWN;
                }
                if (i < ipVector.size() - 1) // avoid delim at the end
                {
                    nit->second += NSNicInfo::DELIM;
                }
            }
        } while (0);
    }
    else
    {
      bprocessed = false;
      m_ErrMsg = "failed to get ";
      m_ErrMsg += IPADDRESSCOLUMNNAME;
      m_ErrMsg += " from wmi network adapter configuration class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(IPSUBNETCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        std::pair<AttributesIter_t, bool> pr = p->m_Attributes.insert(std::make_pair(NSNicInfo::IP_SUBNET_MASKS, std::string()));
        AttributesIter_t nit = pr.first;
        std::vector<std::string> subnetVector;
        CopyFromBstrSafeArrayToOutParams(&vtProp, nit->second, NSNicInfo::DELIM, true, subnetVector);
        VariantClear(&vtProp);

        ssmessage << IPSUBNETCOLUMNNAME << " : ";
        std::vector<std::string>::iterator subnetvecIter = subnetVector.begin();
        for (/**/; subnetvecIter != subnetVector.end(); subnetvecIter++)
            ssmessage << *subnetvecIter << ", ";
        ssmessage << std::endl;
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += IPSUBNETCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(DEFAULTIPGATEWAYCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        std::pair<AttributesIter_t, bool> pr = p->m_Attributes.insert(std::make_pair(NSNicInfo::DEFAULT_IP_GATEWAYS, std::string()));
        AttributesIter_t nit = pr.first;
        std::vector<std::string> gtwVector;
        CopyFromBstrSafeArrayToOutParams(&vtProp, nit->second, NSNicInfo::DELIM, true, gtwVector);
        VariantClear(&vtProp);

        ssmessage << DEFAULTIPGATEWAYCOLUMNNAME << " : ";
        std::vector<std::string>::iterator gtwvecIter = gtwVector.begin();
        for (/**/; gtwvecIter != gtwVector.end(); gtwvecIter++)
            ssmessage << *gtwvecIter << ", ";
        ssmessage << std::endl;
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += DEFAULTIPGATEWAYCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(DNSSERVERSEARCHORDERCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        std::pair<AttributesIter_t, bool> pr = p->m_Attributes.insert(std::make_pair(NSNicInfo::DNS_SERVER_ADDRESSES, std::string()));
        AttributesIter_t nit = pr.first;
        std::vector<std::string> dnsVector;
        CopyFromBstrSafeArrayToOutParams(&vtProp, nit->second, NSNicInfo::DELIM, true, dnsVector);
        VariantClear(&vtProp);
        ssmessage << DNSSERVERSEARCHORDERCOLUMNNAME << " : ";
        std::vector<std::string>::iterator dnsvecIter = dnsVector.begin();
        for (/**/; dnsvecIter != dnsVector.end(); dnsvecIter++)
            ssmessage << *dnsvecIter << ", ";
        ssmessage << std::endl;
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += DNSSERVERSEARCHORDERCOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
    }

    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(DNSHOSTNAMECOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            p->m_Attributes.insert(std::make_pair(NSNicInfo::DNS_HOST_NAME, W2A(V_BSTR(&vtProp))));
            ssmessage << DNSHOSTNAMECOLUMNNAME << " : " << W2A(V_BSTR(&vtProp)) << std::endl;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += DNSHOSTNAMECOLUMNNAME;
        m_ErrMsg += " from wmi network adapter configuration class";
    }

    m_TraceMsg = ssmessage.str();

    return bprocessed;
}

bool InstalledProductsProcessor::Process(IWbemClassObject *precordobj)
{
    if( precordobj == 0 )
    {
        m_ErrMsg = "record object is not specified for wmi product class class" ;
        return false; 
    }
    const char INSTALLEDPRODUCT_PACKAGENAME_COLUMNNAME[] = "PackageName";
    const char INSTALLEDPRODUCT_NAME_COLUMNNAME[] = "Name";
    const char INSTALLEDPRODUCT_VENDOR_COLUMNNAME[] = "Vendor";
    const char INSTALLEDPRODUCT_VERSION_COLUMNNAME[] = "Version";
    const char INSTALLEDPRODUCT_INSTALLPATH_COLUMNNAME[] = "InstallLocation" ;
    bool bprocessed = true ;
    USES_CONVERSION;
    VARIANT vtProp;
    HRESULT hrCol;
    VariantInit(&vtProp);
    InstalledProduct product ;
    hrCol = precordobj->Get(A2W(INSTALLEDPRODUCT_PACKAGENAME_COLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            product.pkgname = W2A(V_BSTR(&vtProp)) ;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += INSTALLEDPRODUCT_PACKAGENAME_COLUMNNAME;
        m_ErrMsg += " from wmi product class";
        return bprocessed;
    }
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(INSTALLEDPRODUCT_NAME_COLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            product.name = W2A(V_BSTR(&vtProp)) ;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += INSTALLEDPRODUCT_NAME_COLUMNNAME;
        m_ErrMsg += " from wmi product class";
        return bprocessed;
    }
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(INSTALLEDPRODUCT_VENDOR_COLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            product.pkgname = W2A(V_BSTR(&vtProp)) ;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += INSTALLEDPRODUCT_VENDOR_COLUMNNAME;
        m_ErrMsg += " from wmi product class";
        return bprocessed;
    }
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(INSTALLEDPRODUCT_VERSION_COLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            product.version = W2A(V_BSTR(&vtProp)) ;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += INSTALLEDPRODUCT_VERSION_COLUMNNAME;
        m_ErrMsg += " from wmi product class";
        return bprocessed;
    }
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(INSTALLEDPRODUCT_INSTALLPATH_COLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            product.installLocation = W2A(V_BSTR(&vtProp)) ;
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += INSTALLEDPRODUCT_INSTALLPATH_COLUMNNAME;
        m_ErrMsg += " from wmi product class";
        return bprocessed;
    }
    m_InstalledProducts->push_back( product ) ;
    return bprocessed;
}

bool QuickFixEngineeringProcessor::Process(IWbemClassObject *precordobj)
{
    if( precordobj == 0 )
    {
        m_ErrMsg = "record object is not specified for wmi quick fix engglass" ;
        return false; 
    }
    const char HOTFIXIDCOLUMNNAME[] = "HotFixID";
    bool bprocessed = true ;
    USES_CONVERSION;
    VARIANT vtProp;
    HRESULT hrCol;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(HOTFIXIDCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            m_WMIHotFixInfoClass->hotfixList.push_back(W2A(V_BSTR(&vtProp)));
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += HOTFIXIDCOLUMNNAME;
        m_ErrMsg += " from wmi quick fix engg class";
        return bprocessed;
    }
    return bprocessed ;
}
bool BaseBoardProcessor::Process(IWbemClassObject *precordobj)
{
    if( precordobj == 0 )
    {
        m_ErrMsg = "record object is not specified for wmi bios class";
        return false;
    }
    const char BASEBOARDCOLUMNNAME[] = "SerialNumber";
    bool bprocessed = true ;
    USES_CONVERSION;
    VARIANT vtProp;
    HRESULT hrCol;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(BASEBOARDCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            m_wmibasebrdclass->serialNo = W2A(V_BSTR(&vtProp));
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += BASEBOARDCOLUMNNAME;
        m_ErrMsg += " from wmi bios class";
        return bprocessed;
    }
    return bprocessed ;
}

bool BIOSRecordProcessor::Process(IWbemClassObject *precordobj)
{
    if( precordobj == 0 )
    {
        m_ErrMsg = "record object is not specified for wmi bios class";
        return false;
    }
    const char BIOSSERIALCOLUMNNAME[] = "SerialNumber";
    bool bprocessed = true ;
    USES_CONVERSION;
    VARIANT vtProp;
    HRESULT hrCol;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(BIOSSERIALCOLUMNNAME), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            m_wmibiosclass->serialNo = W2A(V_BSTR(&vtProp));
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += BIOSSERIALCOLUMNNAME;
        m_ErrMsg += " from wmi bios class";
        return bprocessed;
    }
    return bprocessed ;
}

bool WmiComputerSystemProductProcessor::Process(IWbemClassObject *precordobj)
{
    if (precordobj == 0)
    {
        m_ErrMsg = "record object is not specified for wmi computer system product class";
        return false;
    }
    const char UUID[] = "UUID";
    bool bprocessed = true;
    USES_CONVERSION;
    VARIANT vtProp;
    HRESULT hrCol;
    VariantInit(&vtProp);
    hrCol = precordobj->Get(A2W(UUID), 0, &vtProp, 0, 0);
    if (!FAILED(hrCol))
    {
        if (VT_BSTR == V_VT(&vtProp))
        {
            m_wmicsproductclass->uuid = W2A(V_BSTR(&vtProp));
        }
        VariantClear(&vtProp);
    }
    else
    {
        bprocessed = false;
        m_ErrMsg = "failed to get ";
        m_ErrMsg += UUID;
        m_ErrMsg += " from wmi computer system product class";
        return bprocessed;
    }
    return bprocessed;
}

bool WmiSystemEnclosureProcessor::Process(IWbemClassObject *precordobj)
{
    USES_CONVERSION;
    std::stringstream   ssError;

    if (NULL == precordobj)
    {
        m_ErrMsg = "record object is not specified for wmi SystemEnclosure class";
        DebugPrintf(SV_LOG_ERROR, m_ErrMsg.c_str());
        return false;
    }

    CComVariant     vtSMBIOSAssetTag;
    HRESULT         hr;

    hr = precordobj->Get(L"SMBIOSAssetTag", 0, &vtSMBIOSAssetTag, 0, 0);
    if (FAILED(hr)) {
        ssError << "Failed to get SMBIOSAssetTag from Win32_SystemEnclosure hr=" << std::hex << hr;
        m_ErrMsg = ssError.str();
        DebugPrintf(SV_LOG_ERROR, m_ErrMsg.c_str());
        return false;
    }

    if (VT_BSTR != V_VT(&vtSMBIOSAssetTag)) {
        ssError << "SMBIOSAssetTag from Win32_SystemEnclosure has unexpected type=" << V_VT(&vtSMBIOSAssetTag);
        m_ErrMsg = ssError.str();
        DebugPrintf(SV_LOG_ERROR, m_ErrMsg.c_str());
        return false;
    }

    m_SMBIOSAssetTag = W2A(vtSMBIOSAssetTag.bstrVal);
    return true;
}
