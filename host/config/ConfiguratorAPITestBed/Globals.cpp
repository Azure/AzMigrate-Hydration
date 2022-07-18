//#include <stdafx.h>
#include <iostream>
#include "Globals.h"
#include "localconfigurator.h"

using namespace std;

struct GBLS gbls;
string obj1;
string func1;
string targetdrive = "K";
int cmdargs;

void InitializeGlobalVariables()
{
    LocalConfigurator localConfigurator;
    gbls.ipAddress = GetCxIpAddress().c_str();
    gbls.hostId = localConfigurator.getHostId();
    CmdArgsMap["-obj"] = object;
    CmdArgsMap["-mtd1"] = method1;
    CmdArgsMap["-mtd2"] = method2;
    CmdArgsMap["-tdrive"] = targetdrivename;
}

void cmdLineParser(int argc, char** argv)
{
    // No arguments to parse
    if(argc == 1)
    {
        char reply;
        cout << "No arguments to parse. Do you want to continue with standard tests?:[Y|y] ";
        cin >> reply;
        if((reply == 'Y') || (reply == 'y'))
        {
            return;
        }
        else
        {
           exit(0);
        }
    }
    else
    {
        int argcnt = 0;
        if(argc > 1)
        {
            string tempobj = "";
            int cnt =1;

            while(cnt < argc )
            {
                switch(CmdArgsMap[argv[cnt++]])
                {
                case object:
                    tempobj = argv[cnt++];
                    obj1 = "::" + tempobj;
                    break;
                case method1:
                    func1 = argv[cnt++];
                    break;
                case targetdrivename:
                    targetdrive = argv[cnt++];
                    break;
                default:
                    break;
                } //switch
            } // while
        }
    }
}
