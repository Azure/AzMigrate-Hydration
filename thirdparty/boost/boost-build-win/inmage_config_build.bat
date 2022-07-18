@echo  off

rem currently only works with Visual Studio 2008
rem could be made to work with different vsersions 
rem or a new bat file can be created for each Visual Studio 
rem version that needs to be supported

setlocal

set progName=%0
set BJAM=b2.exe
rem set ICU_PATH=

if "%1"  == "" goto usage

set boost_clean=
set boost_platform=
set boost_arch=
set boost_config=
set boost_dir=
set boost_need_popd=
set boost_error=
set msvc_version=
set parse_ok=yes

:parseArgs
if "%1" == "" (
    goto verifyBoostDir
) else (
    if "%1" == "--clean" (
        if "%boost_clean%" == "" (
            set boost_clean=yes 
        ) else (
            echo ERROR --clean already specified
        )
    ) else (
        if "%1" == "-d" (
            if "%boost_root%" == "" (
                set boost_dir=%2
                shift
            ) else (
                echo ERROR -d already specified
            )
        ) else (
            if "%1" == "-p" (
                if "%boost_platform%" == "" (
                    set boost_platform=%2
                    shift
                ) else (
                    echo ERROR -p already specified
                )
            ) else (
                if "%1" == "-c" (
                    if "%boost_config%" == "" (	
                        shift 
                        goto getConfig1
                    ) else (
                        echo ERROR -c already specified
                    )
                ) else (
                    set parse_ok=
                    echo ERROR invalid command line option: "%1"
                )
            )
        )
    )
)

:nextArg
shift
goto parseArgs

:getConfig1
if "%1" == "debug" (
    set boost_config=%1
	 shift
) else (
    if "%1" == "release" (
        set boost_config=%1
        shift
    ) else (
        parse_ok=
        echo ERROR missing or invalid -c value: "%1"
		  goto nextArg
    )
)

if "%1" == "debug" (
    if "%boost_config%" == "release" (
        set boost_config=%boost_config%,%1
    )
) else (
    if "%1" == "release" (
        if "%boost_config%" == "debug" (
            set boost_config=%boost_config%,%1
        )
    ) else (
        goto parseArgs
    )
)
goto nextArg

:verifyBoostDir
if "%parse_ok%" == "" goto usage
if "%boost_dir%" == "" (
    echo ERROR missing boost dir
    goto usage
)
pushd %boost_dir%
if %ERRORLEVEL% neq 0 goto badDir
set boost_need_popd=yes
if not exist boost-build.jam goto badDir

if "%boost_config%" == "" (
    if "%boost_clean%" == "" (
        echo ERROR missing configuration
        goto usage
    )
)

rem make sure config is set or doing a clean
if "%boost_config%" == "" (
    if "%boost_clean%" == "" (
        echo ERROR invalid configuration %boost_confg%
        goto usage
    )
)

rem check platform specified or doing clean
if "%boost_platform%" == "" (
    if "%boost_clean%" == "" (
        echo ERROR missing platform 
        goto usage
    )
)
if not "%boost_clean%" == "" goto clean

rem make sure platform is supported
if /i "%boost_platform%" == "win32" goto configureWin32
if /i "%boost_platform%" == "x64" goto configureX64
if /i "%boost_platform%" == "ia64" goto configureIa64
echo.
echo ERROR: unsupported platfrom %boost_platform%
goto usage

:configureWin32
set boost_arch=x86
set boost_model=
goto build

:configureX64
set boost_arch=x86
set boost_model=address-model=64
goto build

:configureIa64
set boost_arch=ia64
set boost_model=
goto build

:build
rem build bjam
if not exist bootstrap.bat goto :skip_build_bjam
if not exist %BJAM% goto :build_bjam
goto :build_boost

:build_bjam
cmd /c bootstrap.bat
goto :build_boost

:skip_build_bjam
set BJAM=b2.exe
goto :build_boost

:build_boost
rem create the win-user-config.jam
rem would be nice to automatically figure this out without the need for -v option

rem get msvc version (only need major and minor portion)
for /F "usebackq tokens=6" %%i in (`devenv /? ^| findstr Version`) do set msvc_version=%%i
if "%msvc_version%" == "" goto msvcVersion

rem create win-user-config.jam file with msvc version
for /F "tokens=1,2 delims=." %%A in ("%msvc_version%" ) do (
	 echo using msvc : 14.2 ; > win-user-config.jam
)

rem windows uses --layout=versioned so it can use auto-linking, if boost ever supports it with tagged then we can switch to tagged
rem add additional libraries to build as needed. make sure to add the same libraries to the unix/linux inmage_config_build file
rem see boost docs for complete details (--with-locale -sICU_PATH=%ICU_PATH%)
set BJAM_CMD=%BJAM% --layout=versioned --abbreviate-paths --user-config=win-user-config.jam --build-dir=./build/win/%boost_platform% --stagedir=./stage/win/%boost_platform% --with-system --with-thread --with-date_time --with-atomic --with-regex --with-serialization --with-filesystem --disable-icu --with-program_options --with-random architecture=%boost_arch% %boost_model% variant=%boost_config% link=static threading=multi runtime-link=static stage debug-symbols=on specter=on

rem echo command that will be used
echo building %boost_dir% using %BJAM_CMD%

%BJAM_CMD%

if %ERRORLEVEL% neq 0 goto bjamError
goto done

:clean
rem if "" == "%boost_platform%" goto cleanWinAll
rem set buildDir=.\build\win\%boost_platform%
rem set stageDir=.\stage\win\%boost_platform%
rem goto doClean

:cleanWinAll
set buildDir=.\build\win
set stageDir=.\stage\win
goto doClean

:doClean
rem boost does have a clean option, but it requires knowing the full
rem build and stage dirs which we do not always have. for now just
rem delete the build and stage dirs. that should be good enough
echo rmdir /S /Q "%buildDir%"
rmdir /S /Q "%buildDir%" 2> nul
echo rmdir /S /Q "%stageDir%"
rmdir /S /Q "%stageDir%" 2> nul
goto done

:missingCl
echo.
echo ERROR you must setup the MSVC enviroment before attempting to build boost
goto usage

:msvcVersion
echo.
echo ERROR unable to determin MSVC versions. Make sure that devenv.exe is in the path
goto usage

:badDir
echo.
echo ERROR %boost_dir% does not appear to be a valid boost directory
echo.
goto usage

:usage
echo.
echo usage: %progName% ^-d ^<boost dir^> ^-p ^<platform^> ^-c ^<configuration^> ^| ^-d ^<boost dir^> ^-^-clean
echo.
echo   ^-d ^<boost dir^>    : boost directory to build
echo   ^-p ^<platform^>     : should be either win32, x64, or ia64
echo   ^-c ^<configuration^>: should be either debug or release
echo                       you can specify both in a single pass by listing both after the -c use a common ('.') or space between them
echo                       e.g. 
echo                         -c release,debug
echo                         -c release debug
echo.
set boost_error=true
goto done

:bjamError
echo.
echo Boost build failed
echo.
set boost_error=true
goto done

:done
echo.
if not "%boost_need_popd%" == "" popd
if "%boost_error%" == "" (
    exit /B 0
) else (
    exit /B 1
)

