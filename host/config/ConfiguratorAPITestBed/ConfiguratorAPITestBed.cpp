// ConfiguratorAPITestBed.cpp : Defines the entry point for the console application.
//
#ifdef SV_WINDOWS
#include "stdafx.h"
#endif
#include <ace/Init_ACE.h>
#include <ace/OS_main.h>
#include <cstdlib>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include "sigslot.h"
#include "portablehelpers.h"
#include "dataprotectionexception.h"
#include "configurator2.h"
#include "localconfigurator.h"
#include "TestVolumeManager.h"
#include "Globals.h"
#include "TestSnapshotAPIs.h"
#include "../tal/HandlerCurl.h"
#include "inmsafecapis.h"
#include "terminateonheapcorruption.h"
#include "AgentHealthContract.h"
#include <iostream>

using namespace std;

Configurator* TheConfigurator = 0;
extern string targetdrive;

void run(int argc, char *argv[]);
void usage(char *exename)
{
    stringstream out;
    out << "usage: " << exename << " --default --fromcache\n"
        <<"         "<<exename << " --SourceHealth\n"
        << "       " << exename << " --custom --ip <cx-ip> --port <cx-port> --hostid <hostid>\n"
        << "\n where --fromcache is optional, this option should given if we want to fetch data from cache\n";
    cout << out.str().c_str();
}
int tabs = 0;

void PopulateVSSWriter(SourceAgentProtectionPairHealthIssues & sapphl)
{
	//VSS Writer Health Issue
	std::vector<HealthIssue> aglHealthIssues;
	std::map<std::string, std::string> vssWriterParams1;
	vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::HealthCode,"ECH00031" });
	vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::ErrorCode,"0x8004230c" });
	vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::Operation,"Snapshot" });
	vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::WriteName,"SQL VSS Writer" });
	HealthIssue hiVssWriter1(std::string("ECH00031"), vssWriterParams1);
	aglHealthIssues.push_back(hiVssWriter1);
	sapphl.HealthIssues = aglHealthIssues;
	cout << endl << "Posting VSS Writer Health Issue" << endl;
}

void PopulateVSSProvider(SourceAgentProtectionPairHealthIssues & sapphl)
{
	//VSS Provider Health Issue
	std::vector<HealthIssue> aglHealthIssues;
	HealthIssue hiVssProvider(std::string("ECH00030"), std::map<std::string, std::string>());
	aglHealthIssues.push_back(hiVssProvider);//agent level health issues
	sapphl.HealthIssues = aglHealthIssues;
	cout << endl << "Posting VSS Provider Health Issue..." << endl;
}

void PopulatePeakChurn(SourceAgentProtectionPairHealthIssues & sapphl,std::vector<std::string> &vDiskIds,bool canDiskHealthIssuesBePosted)
{
	if (!canDiskHealthIssuesBePosted)
		return;
	std::vector<HealthIssue> aglHealthIssues;
	std::vector<AgentDiskLevelHealthIssue> dlHealthIssues;//disk level health issues
	std::map<std::string, std::string>peakChurnParams;
	for (int i = 0; i < vDiskIds.size(); i++)
	{
		int nChurn = 60 % std::rand();
		int nTotalChurn = 60 + nChurn;
		std::string strPeakChurn = boost::lexical_cast<std::string>(nTotalChurn);
		//peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::HealthCode,"SourceAgentPeakChurnObserved" });
		peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::ChurnRate,strPeakChurn });
		peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::UploadPending,"200 MB" });
		peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::ObservationTime,"2019-Jul-15 18:41:00 UTC" });
		AgentDiskLevelHealthIssue hiPeakChurn(vDiskIds[i], AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::HealthCode, peakChurnParams);
		dlHealthIssues.push_back(hiPeakChurn);
		sapphl.DiskLevelHealthIssues = dlHealthIssues;
		peakChurnParams.clear();
	}
	cout << endl << "Posting Peak Churn Health Issue..." << endl;
}

void PopulateHighLatency(SourceAgentProtectionPairHealthIssues & sapphl, std::vector<std::string> &vDiskIds, bool canDiskHealthIssuesBePosted)
{
	if (!canDiskHealthIssuesBePosted)
		return;
	std::vector<HealthIssue> aglHealthIssues;
	std::vector<AgentDiskLevelHealthIssue> dlHealthIssues;//disk level health issues
	std::map<std::string, std::string>highLatencyParams;
	for (int i = 0; i < vDiskIds.size(); i++)
	{
		int nLatencyChurn = 200 % std::rand();
		int nTotalLatencyChurn = 60 + nLatencyChurn;
		std::string strHighLatencyChurn = boost::lexical_cast<std::string>(nTotalLatencyChurn);
		//highLatencyParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::HealthCode,"SourceAgentHighLatencyObserved" });
		highLatencyParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::UploadPending,strHighLatencyChurn });
		highLatencyParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::ObservationTime,"2019-Jun-18 19:21:00 UTC" });
		AgentDiskLevelHealthIssue hiHighLatency(vDiskIds[i], AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::HealthCode, highLatencyParams);
		dlHealthIssues.push_back(hiHighLatency);
		sapphl.DiskLevelHealthIssues = dlHealthIssues;
		highLatencyParams.clear();
	}
	cout << endl << "Posting High Latency Health Issue..." << endl;
}

void PostHealthIssues(SourceAgentProtectionPairHealthIssues & sapphl)
{
	TheConfigurator->getVxAgent().ReportAgentHealthStatus(sapphl);
}

void ClearAllHealthIssues(SourceAgentProtectionPairHealthIssues & sapphl)
{
	cout << endl << "Clearing All the Health Issues..." << endl;
	sapphl.HealthIssues.clear();
	sapphl.DiskLevelHealthIssues.clear();
	TheConfigurator->getVxAgent().ReportAgentHealthStatus(sapphl);
}

int main( int argc, char* argv[] )
{
	TerminateOnHeapCorruption();

    init_inm_safe_c_apis();
	// PR#10815: Long Path support
	ACE::init();
    run(argc, argv);
    return 0;
}
void run(int argc, char *argv[])
{
    tal::GlobalInitialize();
    boost::shared_ptr<void> guardTal( static_cast<void*>(0), boost::bind(tal::GlobalShutdown) );

    MaskRequiredSignals();
	LocalConfigurator lconfigurator ;
    std::map<string, string> options;
    do
    {
        if (argc < 2)
        {
            usage(argv[0]);
            break;
        }

        if ((0 == stricmp(argv[1], "--h")) || (0 == stricmp(argv[1], "--help" )))
        {
            usage(argv[0]);
            break;
        }
        else if (0 == stricmp(argv[1], "--default"))
        {
            try
            {
				ConfigSource configSource = FETCH_SETTINGS_FROM_CX;
				if ((argc > 2) && (0 == stricmp(argv[2], "--fromcache")))
				{
					cout << "Parsing cache setting \n\n";
					configSource = USE_ONLY_CACHE_SETTINGS;
				}
                cout << "PARSING INITIAL SETTINGS START" << endl;
                cout << "------------------------------" << endl;
                cout << endl << endl;
                //Configurator * configurator = 0;
                std::string configcachepath ;
                SerializeType_t serializerType = lconfigurator.getSerializerType() ;
            
                if (!InitializeConfigurator(&TheConfigurator,configSource, serializerType,configcachepath))
                {
                    std::cout << "Failed to get the configurator" << std::endl;
                    break;
                }
				TheConfigurator->GetCurrentSettings() ;
                cout << "PARSING INITIAL SETTINGS END" << endl;
                cout << "----------------------------" << endl;
                //TheConfigurator = configurator;
            }
            catch( std::exception const& e ) {
                std::cout << "exception: " << e.what() << std::endl;
                break;
            }
        }
        else if (0 == stricmp(argv[1], "--custom"))
        {
            if (argc != 8) {
                usage(argv[0]);
                break;
            }
            for (int i=2;i < 8; i+=2)
                options[argv[i]] = argv[i+1];
            std::map<string, string>::iterator endit = options.end();
            if ((options.find("--ip") == endit) || (options.find("--port") == endit) ||
                (options.find("--hostid") == endit))
            {
                usage(argv[0]);
                break;
            }
            try
            {
                cout << "PARSING INITIAL SETTINGS START" << endl;
                cout << "------------------------------" << endl;
                int const port = atoi(options["--port"].c_str());
                if(!InitializeConfigurator(&TheConfigurator,options["--ip"], port, options["--hostid"] ))
                {
                    std::cout << "Failed to get configurator" << std::endl;
                    break;
                }
                cout << "PARSING INITIAL SETTINGS END" << endl;
                cout << "----------------------------" << endl;
                cout << endl;
                //TheConfigurator = configurator;
            }
            catch( std::exception const& e ) {
                std::cout << "exception: " << e.what() << std::endl;
                break;
            }
        }
        else if (boost::iequals(std::string(argv[1]), std::string("--SourceHealth")))
        {
            ConfigSource configSource = FETCH_SETTINGS_FROM_CX;
            cout << "Parsing cache setting \n\n";
            configSource = USE_ONLY_CACHE_SETTINGS;
            std::string configcachepath;
            SerializeType_t serializerType = lconfigurator.getSerializerType();

            if (!InitializeConfigurator(&TheConfigurator, configSource, serializerType, configcachepath))
            {
                std::cout << "Failed to get the configurator" << std::endl;
                break;
            }
			bool bV2AProvider = true;
			char choice_provider;
			std::string strHostId;
			std::cout << std::endl << "Is this for V2A (y/n)(default:y)? "; std::cin >> choice_provider;
			if ((choice_provider == 'y') || (choice_provider == 'Y'))
			{
				bV2AProvider = true;
			}
			else
			{
				bV2AProvider = false;
				std::cout << std::endl << "Enter HostId of the Protected Source Machine : "; std::cin >> strHostId;
			}
            char option;
            int numOfDisks;
            std::vector<std::string>vDiskSignatures;
            std::string strDiskSignature;
            bool bCanDiskLevelHealthIssuesbeGenerated = true;
            cout << "Do you want to generate Disk Level Health Issues. (y/n)?"; cin >> option;
            if ((option == 'y') || (option == 'Y'))
            {
                cout << endl << "On How many disks you want to generate Disk Level Health Issues?"; cin >> numOfDisks;
                for (int i = 0; i < numOfDisks; i++)
                {
                    cout << endl << "Enter the disk signature of Disk" << i << ":"; cin >> strDiskSignature;
                    strDiskSignature = std::string("{") + strDiskSignature + std::string("}");
                    cout << endl << "Disk Id:" << strDiskSignature;
                    vDiskSignatures.push_back(strDiskSignature);
                }
            }
            else
            {
                cout << endl << "No Disk Level Health Issues can be generated." << endl;
                bCanDiskLevelHealthIssuesbeGenerated = false;
            }
            bool bHealthIssues = true;
            do
            {
                cout << "Choose options for issuing Health Issues to Cx >> " << endl;
                cout << "(0) -- Clear all Health Issues." << endl;
                cout << "(1) -- VM Level: VSS Writer Health Issue." << endl;
                cout << "(2) -- VM Level: VSS Provider Health Issue." << endl;
                cout << "(3) -- Disk Level: Peak Churn Health Issue." << endl;
                cout << "(4) -- Disk Level: High Latency Health Issue." << endl;
                cout << "(5) -- Disk Level: Peak Churn + High Latency Health Issue." << endl;
                cout << "(6) -- VM + Disk Level: Peak Churn + VSS Writer Health Issue." << endl;
                cout << "(7) -- VM + Disk Level: Peak Churn + VSS Provider Health Issue." << endl;
                cout << "(8) -- VM + Disk Level: High Latency + VSS Writer Health Issue." << endl;
                cout << "(9) -- VM + Disk Level: High Latency + VSS Provider Health Issue." << endl;
                cout << "(10) -- VM + Disk Level: High Latency + Peak Churn + VSS Writer Health Issue." << endl;
                cout << "(11) -- VM + Disk Level: High Latency + Peak Churn + VSS Provider Health Issue." << endl;
                cout << "(12) -- VM + Disk Level: High Latency + Peak Churn + VSS Writer + VSS Provider Health Issue." << endl;
                cout << "(20) -- Do you want to continue?  \n"<< endl;
                cout << "\nEnter option : >>";
                int choose;
                cin >> choose;
                switch (choose)
                {
                    case 0://"(0) -- Clear all Health Issues"
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        ClearAllHealthIssues(Health);
                        break;
                    }
                    case 1://"(1) -- VM Level: VSS Writer Health Issue."
                    {
                        //VSS Writer Health Issue
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateVSSWriter(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 2://"(2) -- VM Level: VSS Provider Health Issue."
                    {
                        //Vss Provider Health Issue
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateVSSProvider(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 3://"(3) -- Disk Level: Peak Churn Health Issue."
                    {
                        //Peak Churn Health Issue
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulatePeakChurn(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 4://"(4) -- Disk Level: High Latency Health Issue." 
                    {
                        //High Latency Health Issue
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateHighLatency(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 5://"(5) -- Disk Level: Peak Churn + High Latency Health Issue." 
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulatePeakChurn(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateHighLatency(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 6://"(6) -- VM + Disk Level: Peak Churn + VSS Writer Health Issue."
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulatePeakChurn(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateVSSWriter(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 7://"(7) -- VM + Disk Level: Peak Churn + VSS Provider Health Issue."
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulatePeakChurn(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateVSSProvider(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 8://"(8) -- VM + Disk Level: High Latency + VSS Writer Health Issue."
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateHighLatency(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateVSSWriter(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 9://"(9) -- VM + Disk Level: High Latency + VSS Provider Health Issue." 
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateHighLatency(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateVSSProvider(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 10://"(10) -- VM + Disk Level: High Latency + Peak Churn + VSS Writer Health Issue." 
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateHighLatency(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulatePeakChurn(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateVSSWriter(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 11://"(11) -- VM + Disk Level: High Latency + Peak Churn + VSS Provider Health Issue."
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateHighLatency(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulatePeakChurn(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateVSSProvider(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 12://"(12) -- VM + Disk Level: High Latency + Peak Churn + VSS Writer + VSS Provider Health Issue."
                    {
                        SourceAgentProtectionPairHealthIssues Health;
                        PopulateHighLatency(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulatePeakChurn(Health, vDiskSignatures, bCanDiskLevelHealthIssuesbeGenerated);
                        PopulateVSSWriter(Health);
                        PopulateVSSProvider(Health);
                        //PostHealthIssues(Health,strHostId,bV2AProvider);
                        PostHealthIssues(Health);
                        break;
                    }
                    case 20://"(20) -- VM Level: VSS Writer Health Issue."
                    {
                        char opt;
                        cout << endl << "Do you want to continue(y:Exit/n:Continue)? "; cin >> opt;
                        if ((opt == 'y') || (opt == 'Y'))
                        {
                            bHealthIssues = false;
                            exit(0);
                        };
                        break;
                    }
                }
            } while (bHealthIssues);
        }
        else
        {
            cout << "Error..." << endl;
            usage(argv[0]);
            break;
        }

        cout << "Choose options for getting Cx settings >> "  << endl;
        cout << "(1) -- All settings" << endl;
        cout << "(2) -- Initial Settings" << endl;
        cout << "(3) -- Snapshot Settings" << endl;
        cout << "(4) -- getReplicationPairInfo" << endl;
        cout << "(5) -- postHealthIssues" << endl;
        cout << "\nEnter option : >>";
        int choose;
        cin >> choose;
        switch (choose)
        {
        case 1:
            {
                TestGetInitialSettings* tgis = static_cast<TestGetInitialSettings*>(TheConfigurator);
                if (tgis)
                    tgis->Print();
                TestSnapshotAPIs tSnapApis;
                tSnapApis.TestGetSnapshotRequestFromCx();
                break;
            }
        case 2:
            {
                TestGetInitialSettings* tgis = static_cast<TestGetInitialSettings*>(TheConfigurator);
                if (tgis)
                    tgis->Print();
                break;
            }
        case 3:
            {
                TestSnapshotAPIs tSnapApis;
                tSnapApis.TestGetSnapshotRequestFromCx();
                break;
            }
        case 4:
            {
                std::map<std::string, std::string> map_info;
                map_info = TheConfigurator->getReplicationPairInfo("imitswks105");
                std::map<std::string, std::string>::iterator it = map_info.begin();
                for (;it != map_info.end(); it++)
                    cout << "first = " << it->first << ", second = " << it->second << endl;
            }
        case 5:
        {
            SourceAgentProtectionPairHealthIssues agentHealth;
            std::vector<HealthIssue> aglHealthIssues;
            //Vss Writer HealthIssue
            std::map<std::string, std::string> vssWriterParams1;
            vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::HealthCode,"ECH00031" });
            vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::ErrorCode,"0x8004230c" });
            vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::Operation,"Snapshot" });
            vssWriterParams1.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::WriteName,"SQL VSS Writer" });
            //HealthIssue hiVssWriter1(std::string("ECH00031"), vssWriterParams1);

            //aglHealthIssues.push_back(hiVssWriter1);
            //agentHealth.HealthIssues = aglHealthIssues;
            //cout << endl << "Posting only one Agentlevel Health Issue..." << endl;
            //TheConfigurator->getVxAgent().ReportAgentHealthStatus(agentHealth);

            //Vss Writer Health Issue;
            std::map<std::string, std::string> vssWriterParams2;
            vssWriterParams2.insert({AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::HealthCode,"ECH00031"});
            vssWriterParams2.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::ErrorCode,"0x8004233c" });
            vssWriterParams2.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::Operation,"Backup" });
            vssWriterParams2.insert({ AgentHealthIssueCodes::VMLevelHealthIssues::VssWriterFailure::WriteName,"Sharepoint VSS Writer" });
            HealthIssue hiVssWriter2(std::string("ECH00031"), vssWriterParams2);
            //aglHealthIssues.push_back(hiVssWriter2);
           // agentHealth.HealthIssues = aglHealthIssues;
            //cout << endl << "Posting two  Agentlevel Health Issues..." << endl;
            //TheConfigurator->getVxAgent().ReportAgentHealthStatus(agentHealth);

           //Vss Provider Health Issue
            HealthIssue hiVssProvider(std::string("ECH00030"), std::map<std::string, std::string>());
            
            
            //aglHealthIssues.push_back(hiVssProvider);//agent level health issues
            //agentHealth.HealthIssues = aglHealthIssues;
            //cout << endl << "Posting three  Agentlevel Health Issues..." << endl;
            //TheConfigurator->getVxAgent().ReportAgentHealthStatus(agentHealth);

            std::vector<AgentDiskLevelHealthIssue> dlHealthIssues;//disk level health issues
            std::map<std::string, std::string>peakChurnParams;
            peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::HealthCode,"SourceAgentPeakChurnObserved" });
            peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::ChurnRate,"60 MBPS" });
            peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::UploadPending,"200 MB" });
            peakChurnParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::ObservationTime,"2019-Jul-15 18:41:00" });
            AgentDiskLevelHealthIssue hiPeakChurn(std::string("{4087759736}"), AgentHealthIssueCodes::DiskLevelHealthIssues::PeakChurn::HealthCode, peakChurnParams);
            dlHealthIssues.push_back(hiPeakChurn);

            agentHealth.DiskLevelHealthIssues = dlHealthIssues;
            cout << endl << "Posting 3 Agent Level and 1 Disk Level Health Issues..." << endl;
            TheConfigurator->getVxAgent().ReportAgentHealthStatus(agentHealth);


            /*std::map<std::string, std::string>highLatencyParams;
            highLatencyParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::HealthCode,"SourceAgentHighLatencyObserved"});
            highLatencyParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::UploadPending,"205 MB" });
            highLatencyParams.insert({ AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::ObservationTime,"2019-Jun-18 19:21:00" });
            AgentDiskLevelHealthIssue hiHighLatency(std::string("{3C48DD76-BE19-42A8-A8C7-5FD182E8E64B}"), AgentHealthIssueCodes::DiskLevelHealthIssues::HighLatency::HealthCode, highLatencyParams);
            dlHealthIssues.push_back(hiHighLatency);

            agentHealth.DiskLevelHealthIssues = dlHealthIssues;
            agentHealth.HealthIssues = aglHealthIssues;

            cout << endl << "Posting 3 Agent Level and 2 Disk Level Health Issues..." << endl;			
            TheConfigurator->getVxAgent().ReportAgentHealthStatus(agentHealth);*/
        }
            break;
        default:
            break;
        }
    }
    while (false);
}

