#ifndef INM_WMIUSERS_H
#define INM_WMIUSERS_H

#include "genericwmi.h"
#include "volumegroupsettings.h"
#include "portablehelpersmajor.h" 
class WmiComputerSystemRecordProcessor : public GenericWMI::WmiRecordProcessor
{
public:
	struct WmiComputerSystemClass
	{
		std::string model;
		std::string manufacturer;

		void Print(void);
	};

	WmiComputerSystemRecordProcessor(WmiComputerSystemClass *p)
		:m_pWmiComputerSystemClass(p)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

private:
   	WmiComputerSystemClass *m_pWmiComputerSystemClass;
	std::string m_ErrMsg;
};

class WmiNetworkAdapterRecordProcessor : public GenericWMI::WmiRecordProcessor
{
public:
	WmiNetworkAdapterRecordProcessor(NicInfos_t *p)
		:m_pNicInfos(p)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

	std::string GetTraceMsg(void)
	{
		return m_TraceMsg;
	}

private:
	NicInfos_t *m_pNicInfos;
	std::string m_ErrMsg;
	std::string m_TraceMsg;
};


class WmiNetworkAdapterConfRecordProcessor : public GenericWMI::WmiRecordProcessor
{
public:
	WmiNetworkAdapterConfRecordProcessor(NicInfos_t *p)
		:m_pNicInfos(p)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

	std::string GetTraceMsg(void)
	{
		return m_TraceMsg;
	}

private:
	NicInfos_t *m_pNicInfos;
	std::string m_ErrMsg;
	std::string m_TraceMsg;
};


class BIOSRecordProcessor : public GenericWMI::WmiRecordProcessor
{
public:
    struct WMIBIOSClass
    {
        std::string serialNo ;
        void Print(void);
    };

	BIOSRecordProcessor(WMIBIOSClass *p)
		:m_wmibiosclass(p)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

private:
    WMIBIOSClass* m_wmibiosclass;
	std::string m_ErrMsg;
};
class InstalledProductsProcessor : public GenericWMI::WmiRecordProcessor
{
public:
	InstalledProductsProcessor(std::list<InstalledProduct>* installedProducts)
		:m_InstalledProducts(installedProducts)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

private:
	std::list<InstalledProduct>* m_InstalledProducts;
	std::string m_ErrMsg;

} ;

class QuickFixEngineeringProcessor : public GenericWMI::WmiRecordProcessor
{
public:
	struct WMIHotFixInfoClass
	{
		std::list<std::string> hotfixList ;
	} ;
	QuickFixEngineeringProcessor(WMIHotFixInfoClass *p)
		:m_WMIHotFixInfoClass(p)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

private:
	WMIHotFixInfoClass* m_WMIHotFixInfoClass;
	std::string m_ErrMsg;

} ;

class BaseBoardProcessor : public GenericWMI::WmiRecordProcessor
{
public:
    struct WMIBasedBoardClass
    {
        std::string serialNo ;
        void Print(void);
    };

	BaseBoardProcessor(WMIBasedBoardClass *p)
		:m_wmibasebrdclass(p)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

private:
    WMIBasedBoardClass* m_wmibasebrdclass;
	std::string m_ErrMsg;
};

class WmiComputerSystemProductProcessor : public GenericWMI::WmiRecordProcessor
{
public:
	struct WmiComputerSystemProductClass
	{
		std::string uuid;
		void Print(void);
	};

	WmiComputerSystemProductProcessor(WmiComputerSystemProductClass *p)
		:m_wmicsproductclass(p)
	{
	}
	bool Process(IWbemClassObject *precordobj);
	std::string GetErrMsg(void)
	{
		return m_ErrMsg;
	}

private:
	WmiComputerSystemProductClass* m_wmicsproductclass;
	std::string m_ErrMsg;
};

class WmiSystemEnclosureProcessor : public GenericWMI::WmiRecordProcessor
{
public:
    bool Process(IWbemClassObject *precordobj);

    std::string GetErrMsg(void)
    {
        return m_ErrMsg;
    }

    std::string GetSMBIOSAssetTag()
    {
        return m_SMBIOSAssetTag;
    }

private:
    std::string m_SMBIOSAssetTag;
    std::string m_ErrMsg;

};

#endif