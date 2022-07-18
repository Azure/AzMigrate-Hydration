setlocal
set IntDir=%1

set VS_UNICODE_OUTPUT=
copy /Y ..\cdpdef.cpp cdpdeftestbed.cpp
copy /Y ..\cdpsnapshotrequest.cpp cdpsnapshotrequesttestbed.cpp
copy /Y ..\configuratorrpc.cpp configuratorrpctestbed.cpp
copy /Y ..\configurevxagentproxy.cpp configurevxagentproxytestbed.cpp
copy /Y ..\cxconfigurator.cpp cxconfiguratortestbed.cpp
copy /Y ..\fileconfigurator.cpp fileconfiguratortestbed.cpp
copy /Y ..\win\fileconfiguratorex.cpp fileconfiguratorextestbed.cpp
copy /Y ..\initialsettings.cpp initialsettingstestbed.cpp
copy /Y ..\marshal.cpp marshaltestbed.cpp
copy /Y ..\retentionsettings.cpp retentionsettingstestbed.cpp
copy /Y ..\rpcconfigurator.cpp rpcconfiguratortestbed.cpp
copy /Y ..\svconfig.cpp svconfigtestbed.cpp
copy /Y ..\talwrapper.cpp talwrappertestbed.cpp
copy /Y ..\throttleconfigurator.cpp throttleconfiguratortestbed.cpp
copy /Y ..\unmarshal.cpp unmarshaltestbed.cpp
copy /Y ..\volumegroupsettings.cpp volumegroupsettingstestbed.cpp
copy /Y ..\switchinitialsettings.cpp switchinitialsettingstestbed.cpp
copy /Y ..\win\svsemaphore.cpp svsemaphoretestbed.cpp
copy /Y ..\retentioninformation.cpp retentioninformationtestbed.cpp
copy /Y ..\configwrapper.cpp configwrappertestbed.cpp
copy /Y ..\apiwrapper.cpp apiwrapper.cpp
copy /Y ..\xmlwrapper.cpp xmlwrapper.cpp
copy /Y ..\xmlmarshal.cpp xmlmarshal.cpp
copy /Y ..\xmlunmarshal.cpp xmlunmarshal.cpp
copy /Y ..\snapshotmanagerproxy.cpp snapshotmanagerproxy.cpp

if (%IntDir%)==(Debug\) (
    echo /Od /I "..\..\common" /I "..\..\common\win32" /I "..\..\cdplibs" /I "..\..\s2libs\thread" /I "..\..\s2libs\common" /I "..\win" /I "..\." /I "..\..\..\thirdparty\ace-6.4.6\ACE_wrappers" /I "..\..\..\thirdparty\boost\boost_1_78_0" /I "..\..\log" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "_DEBUG" /D "_LIB" /D "SV_WINDOWS=1" /D "_WIN32_WINNT=0x0502" /D "ACE_AS_STATIC_LIBS" /D "DP_PORTABLE" /D "_MBCS" /Gm /EHsc /RTC1 /MTd /GS /Zc:wchar_t /GR /Fo"Debug/" /Fd"Debug/rpcconfig.pdb" /FR"Debug/" /W3 /Zi /TP /Zs /showIncludes /nologo > %IntDir%\makeserialize.rsp
) else (
    echo /O2 /I "..\..\common" /I "..\..\common\win32" /I "..\..\cdplibs" /I "..\..\s2libs\thread" /I "..\..\s2libs\common" /I "..\win" /I "..\." /I "..\..\..\thirdparty\ace-6.4.6\ACE_wrappers" /I "..\..\..\thirdparty\boost\boost_1_78_0" /I "..\..\log" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "SV_WINDOWS=1" /D "WIN32_LEAN_AND_MEAN" /D "_WIN32_WINNT=0x0502" /D "ACE_AS_STATIC_LIBS" /D "_MBCS" /FD /EHsc /MT /GS /Zc:wchar_t /GR /Fo"Release/" /Fd"Release/vc70.pdb" /W3 /WX /Zi /TP /Zs /showIncludes /nologo > %IntDir%\makeserialize.rsp
)

cl.exe @%IntDir%\makeserialize.rsp ..\volumegroupsettings.h > %IntDir%\volumegroupsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc header ..\volumegroupsettings.h --testbed serialize > serializevolumegroupsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc src ..\volumegroupsettings.h --testbed serialize > serializevolumegroupsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\transport_settings.h > %IntDir%\transport_settings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc header ..\transport_settings.h --testbed serialize > serializetransport_settings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc src ..\transport_settings.h --testbed serialize > serializetransport_settingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\retentionsettings.h > %IntDir%\retentionsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc header ..\retentionsettings.h  --testbed serialize > serializeretentionsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc src ..\retentionsettings.h  --testbed serialize > serializeretentionsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\initialsettings.h > %IntDir%\initialsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc header ..\initialsettings.h --testbed serialize > serializeinitialsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc src ..\initialsettings.h --testbed serialize > serializeinitialsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\cdpsnapshotrequest.h > %IntDir%\cdpsnapshotrequest.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc header ..\cdpsnapshotrequest.h --testbed serialize > serializecdpsnapshotrequest.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc src ..\cdpsnapshotrequest.h --testbed serialize > serializecdpsnapshotrequesttestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\atconfigmanagersettings.h > %IntDir%\atconfigmanagersettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc header ..\atconfigmanagersettings.h --testbed serialize > serializeatconfigmanagersettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc src ..\atconfigmanagersettings.h --testbed serialize > serializeatconfigmanagersettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\switchinitialsettings.h > %IntDir%\switchinitialsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\switchinitialsettings.inc header ..\switchinitialsettings.h --testbed serialize > serializeswitchinitialsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\switchinitialsettings.inc src ..\switchinitialsettings.h --testbed serialize > serializeswitchinitialsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\retentioninformation.h > %IntDir%\retentioninformation.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc header ..\retentioninformation.h --testbed serialize > serializeretentioninformation.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc src ..\retentioninformation.h --testbed serialize > serializeretentioninformationtestbed.cpp


cl.exe @%IntDir%\makeserialize.rsp ..\prismsettings.h > %IntDir%\prismsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc header ..\prismsettings.h --testbed serialize > serializeprismsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc src ..\prismsettings.h --testbed serialize > serializeprismsettingstestbed.cpp


cl.exe @%IntDir%\makeserialize.rsp ..\volumegroupsettings.h > %IntDir%\volumegroupsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc header ..\volumegroupsettings.h --testbed xmlize > xmlizevolumegroupsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc src ..\volumegroupsettings.h --testbed xmlize > xmlizevolumegroupsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\transport_settings.h > %IntDir%\transport_settings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc header ..\transport_settings.h --testbed xmlize > xmlizetransport_settings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc src ..\transport_settings.h --testbed xmlize > xmlizetransport_settingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\retentionsettings.h > %IntDir%\retentionsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc header ..\retentionsettings.h  --testbed xmlize > xmlizeretentionsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc src ..\retentionsettings.h  --testbed xmlize > xmlizeretentionsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\initialsettings.h > %IntDir%\initialsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc header ..\initialsettings.h --testbed xmlize > xmlizeinitialsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc src ..\initialsettings.h --testbed xmlize > xmlizeinitialsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\cdpsnapshotrequest.h > %IntDir%\cdpsnapshotrequest.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc header ..\cdpsnapshotrequest.h --testbed xmlize > xmlizecdpsnapshotrequest.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc src ..\cdpsnapshotrequest.h --testbed xmlize > xmlizecdpsnapshotrequesttestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\atconfigmanagersettings.h > %IntDir%\atconfigmanagersettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc header ..\atconfigmanagersettings.h --testbed xmlize > xmlizeatconfigmanagersettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc src ..\atconfigmanagersettings.h --testbed xmlize > xmlizeatconfigmanagersettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\switchinitialsettings.h > %IntDir%\switchinitialsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\switchinitialsettings.inc header ..\switchinitialsettings.h --testbed xmlize > xmlizeswitchinitialsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\switchinitialsettings.inc src ..\switchinitialsettings.h --testbed xmlize > xmlizeswitchinitialsettingstestbed.cpp

cl.exe @%IntDir%\makeserialize.rsp ..\retentioninformation.h > %IntDir%\retentioninformation.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc header ..\retentioninformation.h --testbed xmlize > xmlizeretentioninformation.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc src ..\retentioninformation.h --testbed xmlize > xmlizeretentioninformationtestbed.cpp


cl.exe @%IntDir%\makeserialize.rsp ..\prismsettings.h > %IntDir%\prismsettings.inc
..\%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc header ..\prismsettings.h --testbed xmlize > xmlizeprismsettings.h
..\%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc src ..\prismsettings.h --testbed xmlize > xmlizeprismsettingstestbed.cpp
