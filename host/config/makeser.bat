setlocal
set IntDir=%1

if "%IntDir%"=="Debug\" (
    echo /Od /I "..\appframeworklib" /I "..\appframeworklib\config" /I "..\common" /I "..\common\win32" /I "..\cdpcli" /I "..\s2libs\thread" /I "..\s2libs\common" /I "win" /I "." /I "..\..\thirdparty\ace-6.4.6\ACE_wrappers" /I ".\win" /I "..\..\thirdparty\boost\boost_1_78_0" /I "..\log" /I "..\config" /I "..\config\win" /I "..\cdplibs\\" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "_DEBUG" /D "_LIB" /D "SV_WINDOWS=1" /D "_WIN32_WINNT=0x0502" /D "ACE_AS_STATIC_LIBS" /D "DP_PORTABLE" /D "_MBCS" /Gm /EHsc /RTC1 /MTd /GS /Zc:wchar_t /GR /Fo"Debug/" /FR"Debug/" /W3 /Zi /TP /Zs /showIncludes /nologo > %IntDir%\makeserialize.rsp
) else  (
	if "%IntDir%"=="Release\" (
		echo /O2 /I "..\appframeworklib" /I "..\appframeworklib\config" /I "..\common" /I "..\common\win32" /I "..\cdplibs" /I "..\s2libs\thread" /I "..\s2libs\common" /I "win" /I "." /I "..\..\thirdparty\ace-6.4.6\ACE_wrappers" /I "..\..\thirdparty\boost\boost_1_78_0" /I "..\log" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "SV_WINDOWS=1" /D "WIN32_LEAN_AND_MEAN" /D "_WIN32_WINNT=0x0502" /D "ACE_AS_STATIC_LIBS" /D "_MBCS" /FD /EHsc /MT /GS /Zc:wchar_t /GR /Fo"Release/" /W3 /WX /Zi /TP /Zs /showIncludes /nologo > %IntDir%\makeserialize.rsp
	) else  (
		if "%IntDir%"=="x64\Debug\" (
			echo /Od /I "..\appframeworklib" /I "..\appframeworklib\config" /I "..\common" /I "..\common\win32" /I "..\cdpcli" /I "..\s2libs\thread" /I "..\s2libs\common" /I "win" /I "." /I "..\..\thirdparty\ace-6.4.6\ACE_wrappers" /I ".\win" /I "..\..\thirdparty\boost\boost_1_78_0" /I "..\log" /I "..\config" /I "..\config\win" /I "..\cdplibs\\" /D "WIN32" /D "WIN32_LEAN_AND_MEAN" /D "_DEBUG" /D "_LIB" /D "SV_WINDOWS=1" /D "_WIN32_WINNT=0x0502" /D "ACE_AS_STATIC_LIBS" /D "DP_PORTABLE" /D "_MBCS" /Gm /EHsc /RTC1 /MTd /GS /Zc:wchar_t /GR /Fo"x64/Debug/" /FR"x64/Debug/" /W3 /Zi /TP /Zs /showIncludes /nologo > "%IntDir%"\makeserialize.rsp
		) else  (
			if "%IntDir%"=="x64\Release\" (
				echo /O2 /I "..\appframeworklib" /I "..\appframeworklib\config" /I "..\common" /I "..\common\win32" /I "..\cdplibs" /I "..\s2libs\thread" /I "..\s2libs\common" /I "win" /I "." /I "..\..\thirdparty\ace-6.4.6\ACE_wrappers" /I "..\..\thirdparty\boost\boost_1_78_0" /I "..\log" /D "WIN32" /D "NDEBUG" /D "_LIB" /D "SV_WINDOWS=1" /D "WIN32_LEAN_AND_MEAN" /D "_WIN32_WINNT=0x0502" /D "ACE_AS_STATIC_LIBS" /D "_MBCS" /FD /EHsc /MT /GS /Zc:wchar_t /GR /Fo"x64/Release/" /W3 /WX /Zi /TP /Zs /showIncludes /nologo > %IntDir%\makeserialize.rsp
			)	
		)
	)
)	


set VS_UNICODE_OUTPUT=
cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkvolumegroupsettings.pdb" volumegroupsettings.h > %IntDir%\volumegroupsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc header volumegroupsettings.h --normal serialize > serializevolumegroupsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc src volumegroupsettings.h --normal serialize > serializevolumegroupsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mktransport.pdb" transport_settings.h > %IntDir%\transport_settings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc header transport_settings.h --normal serialize > serializetransport_settings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc src transport_settings.h --normal serialize > serializetransport_settings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkretentionsettings.pdb" retentionsettings.h > %IntDir%\retentionsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc header retentionsettings.h  --normal serialize > serializeretentionsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc src retentionsettings.h  --normal serialize > serializeretentionsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkinitialsettings.pdb" initialsettings.h > %IntDir%\initialsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc header initialsettings.h --normal serialize > serializeinitialsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc src initialsettings.h --normal serialize > serializeinitialsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkcdpsnapshotrequest.pdb" cdpsnapshotrequest.h > %IntDir%\cdpsnapshotrequest.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc header cdpsnapshotrequest.h --normal serialize > serializecdpsnapshotrequest.h
%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc src cdpsnapshotrequest.h --normal serialize > serializecdpsnapshotrequest.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkretentioninformation.pdb" retentioninformation.h > %IntDir%\retentioninformation.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc header retentioninformation.h --normal serialize > serializeretentioninformation.h
%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc src retentioninformation.h --normal serialize > serializeretentioninformation.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkatconfigmanagersettings.pdb" atconfigmanagersettings.h > %IntDir%\atconfigmanagersettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc header atconfigmanagersettings.h --normal serialize > serializeatconfigmanagersettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc src atconfigmanagersettings.h --normal serialize > serializeatconfigmanagersettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkswitchinitialsettings.pdb" switchinitialsettings.h > %IntDir%\switchinitialsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\switchinitialsettings.inc header switchinitialsettings.h --normal serialize > serializeswitchinitialsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\switchinitialsettings.inc src switchinitialsettings.h --normal serialize > serializeswitchinitialsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkpushInstallationSettings.pdb" pushInstallationSettings.h > %IntDir%\pushInstallationSettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\pushInstallationSettings.inc header pushInstallationSettings.h --normal serialize > serializepushInstallationSettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\pushInstallationSettings.inc src pushInstallationSettings.h --normal serialize > serializepushInstallationSettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkapplicationsettings.pdb" applicationsettings.h > %IntDir%\applicationsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\applicationsettings.inc header applicationsettings.h --normal serialize > serializeapplicationsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\applicationsettings.inc src applicationsettings.h --normal serialize > serializeapplicationsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkprismsettings.pdb" prismsettings.h > %IntDir%\prismsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc header prismsettings.h --normal serialize > serializeprismsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc src prismsettings.h --normal serialize > serializeprismsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mksourcesystemconfigsettings.pdb" sourcesystemconfigsettings.h > %IntDir%\sourcesystemconfigsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\sourcesystemconfigsettings.inc header sourcesystemconfigsettings.h --normal serialize > serializesourcesystemconfigsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\sourcesystemconfigsettings.inc src sourcesystemconfigsettings.h --normal serialize > serializesourcesystemconfigsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mktask.pdb" task.h > %IntDir%\task.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\task.inc header task.h --normal serialize > serializetask.h
%IntDir%\makeserialize.exe --includes @%IntDir%\task.inc src task.h --normal serialize > serializetask.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkcompressmode.pdb" compressmode.h > %IntDir%\compressmode.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\compressmode.inc header compressmode.h --normal serialize > serializecompressmode.h
%IntDir%\makeserialize.exe --includes @%IntDir%\compressmode.inc src compressmode.h --normal serialize > serializecompressmode.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkvolumerecoveryhelperssettings.pdb" volumerecoveryhelperssettings.h > %IntDir%\volumerecoveryhelperssettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\volumerecoveryhelperssettings.inc header volumerecoveryhelperssettings.h --normal serialize > serializevolumerecoveryhelperssettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\volumerecoveryhelperssettings.inc src volumerecoveryhelperssettings.h --normal serialize > serializevolumerecoveryhelperssettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkvolumegroupsettings.pdb" volumegroupsettings.h > %IntDir%\volumegroupsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc header volumegroupsettings.h --normal xmlize > xmlizevolumegroupsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\volumegroupsettings.inc src volumegroupsettings.h --normal xmlize > xmlizevolumegroupsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mktransport.pdb" transport_settings.h > %IntDir%\transport_settings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc header transport_settings.h --normal xmlize > xmlizetransport_settings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\transport_settings.inc src transport_settings.h --normal xmlize > xmlizetransport_settings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkretentionsettings.pdb" retentionsettings.h > %IntDir%\retentionsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc header retentionsettings.h  --normal xmlize > xmlizeretentionsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\retentionsettings.inc src retentionsettings.h  --normal xmlize > xmlizeretentionsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkinitialsettings.pdb" initialsettings.h > %IntDir%\initialsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc header initialsettings.h --normal xmlize > xmlizeinitialsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\initialsettings.inc src initialsettings.h --normal xmlize > xmlizeinitialsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkcdpsnapshotrequest.pdb" cdpsnapshotrequest.h > %IntDir%\cdpsnapshotrequest.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc header cdpsnapshotrequest.h --normal xmlize > xmlizecdpsnapshotrequest.h
%IntDir%\makeserialize.exe --includes @%IntDir%\cdpsnapshotrequest.inc src cdpsnapshotrequest.h --normal xmlize > xmlizecdpsnapshotrequest.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkretentioninformation.pdb" retentioninformation.h > %IntDir%\retentioninformation.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc header retentioninformation.h --normal xmlize > xmlizeretentioninformation.h
%IntDir%\makeserialize.exe --includes @%IntDir%\retentioninformation.inc src retentioninformation.h --normal xmlize > xmlizeretentioninformation.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkatconfigmanagersettings.pdb" atconfigmanagersettings.h > %IntDir%\atconfigmanagersettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc header atconfigmanagersettings.h --normal xmlize > xmlizeatconfigmanagersettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\atconfigmanagersettings.inc src atconfigmanagersettings.h --normal xmlize > xmlizeatconfigmanagersettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkpushInstallationSettings.pdb" pushInstallationSettings.h > %IntDir%\pushInstallationSettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\pushInstallationSettings.inc header pushInstallationSettings.h --normal xmlize > xmlizepushInstallationSettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\pushInstallationSettings.inc src pushInstallationSettings.h --normal xmlize > xmlizepushInstallationSettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkapplicationsettings.pdb" applicationsettings.h > %IntDir%\applicationsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\applicationsettings.inc header applicationsettings.h --normal xmlize > xmlizeapplicationsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\applicationsettings.inc src applicationsettings.h --normal xmlize > xmlizeapplicationsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkprismsettings.pdb" prismsettings.h > %IntDir%\prismsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc header prismsettings.h --normal xmlize > xmlizeprismsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\prismsettings.inc src prismsettings.h --normal xmlize > xmlizeprismsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mksourcesystemconfigsettings.pdb" sourcesystemconfigsettings.h > %IntDir%\sourcesystemconfigsettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\sourcesystemconfigsettings.inc header sourcesystemconfigsettings.h --normal xmlize > xmlizesourcesystemconfigsettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\sourcesystemconfigsettings.inc src sourcesystemconfigsettings.h --normal xmlize > xmlizesourcesystemconfigsettings.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mktask.pdb" task.h > %IntDir%\task.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\task.inc header task.h --normal xmlize > xmlizetask.h
%IntDir%\makeserialize.exe --includes @%IntDir%\task.inc src task.h --normal xmlize > xmlizetask.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkcompressmode.pdb" compressmode.h > %IntDir%\compressmode.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\compressmode.inc header compressmode.h --normal xmlize > xmlizecompressmode.h
%IntDir%\makeserialize.exe --includes @%IntDir%\compressmode.inc src compressmode.h --normal xmlize > xmlizecompressmode.cpp

cl.exe @%IntDir%\makeserialize.rsp /Fd"%IntDir%\mkvolumerecoveryhelperssettings.pdb" volumerecoveryhelperssettings.h > %IntDir%\volumerecoveryhelperssettings.inc
%IntDir%\makeserialize.exe --includes @%IntDir%\volumerecoveryhelperssettings.inc header volumerecoveryhelperssettings.h --normal xmlize > xmlizevolumerecoveryhelperssettings.h
%IntDir%\makeserialize.exe --includes @%IntDir%\volumerecoveryhelperssettings.inc src volumerecoveryhelperssettings.h --normal xmlize > xmlizevolumerecoveryhelperssettings.cpp
