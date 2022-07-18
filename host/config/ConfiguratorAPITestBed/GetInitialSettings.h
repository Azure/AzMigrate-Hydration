#ifndef TESTGETINITIALSETTINGS
#define TESTGETINITIALSETTINGS


class TestGetInitialSettings
{
private:
   InitialSettings initialsettings;
   LocalConfigurator localconf; //Must be declared before dpc & tal
   std::string dpc;
   std::string senddata;
   std::string response;
   TalWrapper tal;
   
public:
 TestGetInitialSettings();
 InitialSettings Test_getInitialSettings();
 void Print();
 void PrintCdpsettings();
 void PrintHostVolumeGroupSettings();
 LocalConfigurator& getLocalConfigurator();
};

#endif