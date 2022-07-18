#include "portablehelpers.h"
#include "HttpFileTransfer.h"
#include "curlwrapper.h"
#ifdef WIN32
#include <WinBase.h>
#endif

HttpFileTransfer::HttpFileTransfer(const bool &https,const std::string& cxIp,const std::string& port)
: formpostUpload(NULL),lastptrUpload(NULL),formpostDownload(NULL),lastptrDownload(NULL),
m_Https(https),m_CxIpAddress(cxIp),m_CxPort(port)
{
}

HttpFileTransfer::~HttpFileTransfer()
{
	curl_formfree(formpostUpload);
	curl_formfree(formpostDownload);
}

bool HttpFileTransfer::Init()
{
	bool ret = true ;
	if(CURLE_OK != curl_global_init(CURL_GLOBAL_ALL) )
	{
		ret = false ;
	}
	return ret;
}


bool HttpFileTransfer::Upload(const std::string& remotedir,const std::string& localFilePath)
{
	DebugPrintf(SV_LOG_DEBUG,"Upload Entered\n" );

	std::string csIP;
	int httpPortNumber;
	bool isIpAvailable = true ;
	bool isPortAvailable = true ;
	int responseTimeOut = SV_DEFAULT_TRANSPORT_RESPONSE_TIMEOUT;

	HTTP_CONNECTION_SETTINGS s ;
	Configurator* TheConfigurator = NULL;


	if(!GetConfigurator(&TheConfigurator))
	{
		DebugPrintf(SV_LOG_DEBUG, "Unable to obtain configurator %s %d", FUNCTION_NAME, LINE_NO);
		DebugPrintf(SV_LOG_DEBUG,"EXITED: FileUpload\n" );
		return false;
	}
	if( InmStrCmp<NoCaseCharCmp>(m_CxIpAddress.c_str(),"")== 0 )
		isIpAvailable = false ;

	if(InmStrCmp<NoCaseCharCmp>(m_CxPort.c_str(),"")== 0)
		isPortAvailable = false ;
	if(!isIpAvailable || !isPortAvailable)
		s = TheConfigurator->getHttpSettings();

	if(!isIpAvailable)
	{
		//Find CX Ip from Local Configurtaor
		csIP = s.ipAddress  ;
	}
	else
	{
		csIP = m_CxIpAddress ;
	}
	DebugPrintf(SV_LOG_INFO,"Cx IP address = %s\n",csIP.c_str());
	if(!isPortAvailable)
	{
		//Find CX port number
		httpPortNumber =  s.port ;
	}
	else
	{
		httpPortNumber = boost::lexical_cast<int>(m_CxPort) ;
	}
	DebugPrintf(SV_LOG_INFO,"Cx HTTP Port Number = %d\n",httpPortNumber);


	curl_formadd(&formpostUpload,
		&lastptrUpload,
		CURLFORM_COPYNAME, "file",
		CURLFORM_FILE, localFilePath.c_str(),
		CURLFORM_END);
	curl_formadd(&formpostUpload,
		&lastptrUpload,
		CURLFORM_COPYNAME, "file_path",
		CURLFORM_COPYCONTENTS, remotedir.c_str(),
		CURLFORM_END);


	std::string result;
	CurlOptions options(csIP, httpPortNumber, "/upload_file.php", m_Https);

	options.writeCallback(&result,write_callback_upload);
	options.lowSpeedLimit(16);//appx. 1KB should be transfer in 1Minute
	options.lowSpeedTime(60);//time period during which the transfer should not go below CURLOPT_LOW_SPEED_LIMIT
	options.responseTimeout(responseTimeOut);

	try {
		CurlWrapper cw;
		cw.formPost(options,formpostUpload);
	} catch(ErrorException& exception )
	{
		DebugPrintf(SV_LOG_ERROR, "FAILED : %s with error %s\n",FUNCTION_NAME, exception.what());
		return false;
	}

	DebugPrintf(SV_LOG_DEBUG,"Upload Exited .\n");
	return true ;
}


bool HttpFileTransfer::Download(const std::string& remoteFilePath,const std::string& localFilePath)
{
	DebugPrintf(SV_LOG_DEBUG,"Download Entered .\n");

	bool ret  = true;
	int responseTimeOut = SV_DEFAULT_TRANSPORT_RESPONSE_TIMEOUT;
	std::string csIP ;
	int httpPortNumber ;
	HTTP_CONNECTION_SETTINGS s ;
	bool isIpAvailable = true ;
	bool isPortAvailable = true ;

	std::stringstream ssfiles;
	ssfiles << "For http file transfer download, " << "remote file: " << remoteFilePath << ", "
		<< "local file: " << localFilePath;
	DebugPrintf(SV_LOG_DEBUG, "%s.\n", ssfiles.str().c_str());

	std::string ldirname = PlatformBasedDirname(localFilePath);
	SVERROR svError = SVMakeSureDirectoryPathExists( ldirname.c_str() ) ;
	if (svError.succeeded())
	{
		DebugPrintf(SV_LOG_DEBUG, "local directory %s exists.\n", ldirname.c_str());
	}
	else 
	{
		DebugPrintf(SV_LOG_ERROR, "%s, failed to make sure presence or creation of local directory %s with error %s.\n",
			ssfiles.str().c_str(), ldirname.c_str(), svError.toString());
		return false;
	}

	Configurator* TheConfigurator = NULL;
	if(!GetConfigurator(&TheConfigurator))
	{
		DebugPrintf(SV_LOG_ERROR, "%s, unable to obtain configurator.\n", ssfiles.str().c_str());
		return false;
	}

	if( InmStrCmp<NoCaseCharCmp>(m_CxIpAddress.c_str(),"")== 0 )
		isIpAvailable = false ;

	if(InmStrCmp<NoCaseCharCmp>(m_CxPort.c_str(),"")== 0)
		isPortAvailable = false ;
	if(!isIpAvailable || !isPortAvailable)
		s = TheConfigurator->getHttpSettings();

	if(!isIpAvailable)
	{
		//Find CX Ip from Local Configurtaor
		csIP = s.ipAddress  ;
	}
	else
	{
		csIP = m_CxIpAddress ;
	}
	DebugPrintf(SV_LOG_INFO,"Cx IP address = %s\n",csIP.c_str());
	if(!isPortAvailable)
	{
		//Find CX port number
		httpPortNumber =  s.port ;
	}
	else
	{
		httpPortNumber = boost::lexical_cast<int>(m_CxPort) ;
	}
	DebugPrintf(SV_LOG_INFO,"Cx Port Number = %d\n",httpPortNumber);


	curl_formadd(&formpostDownload,
		&lastptrDownload,
		CURLFORM_COPYNAME, "file_path",
		CURLFORM_COPYCONTENTS, remoteFilePath.c_str(),
		CURLFORM_END);


	FILE *lfstream;
#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
	lfstream = _wfopen(getLongPathName(localFilePath.c_str()).c_str(), ACE_TEXT("wbN"));
#else
	lfstream = fopen(localFilePath.c_str(), "wb");
#endif

	if(NULL == lfstream)
	{
		DebugPrintf(SV_LOG_ERROR, "%s, failed to open file %s for writing with error number %d.\n", ssfiles.str().c_str(), localFilePath.c_str(), errno);
		return false;
	}

	CurlOptions options(csIP,httpPortNumber,"/download_file.php", m_Https);

	options.writeCallback(lfstream,write_callback_download);
	options.lowSpeedLimit(16);
	options.lowSpeedTime(60);
	options.responseTimeout(responseTimeOut);



	try {
		CurlWrapper cw;
		cw.formPost(options,formpostDownload);
	} catch(ErrorException& exception )
	{
		DebugPrintf(SV_LOG_ERROR, "FAILED : %s with error %s\n",FUNCTION_NAME, exception.what());
		ret = false;
	}

	if (0 != fclose(lfstream))
	{
		DebugPrintf(SV_LOG_ERROR, "%s, failed to close file %s with error number %d\n", ssfiles.str().c_str(), localFilePath.c_str(), errno);
		ret = false ;
	}
	if(!ret)
		ACE_OS::unlink(getLongPathName(localFilePath.c_str()).c_str());


	DebugPrintf(SV_LOG_DEBUG,"Download Exited .\n");
	return ret ;
}
