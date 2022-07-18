#include <string>
#include <iostream>
#include <algorithm>

#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>

#include "diskslayoutupdaterimp.h"
#include "configurevxagent.h"
#include "configurecxagent.h"
#include "disklabeldefines.h"
#include "devicestream.h"
#include "filterdrvifmajor.h"
#include "devicefilter.h"
#include "logger.h"
#include "portablehelpers.h"
#include "portablehelpersmajor.h"
#include "dlmapi.h"
#include "volumegroupsettings.h"
#include "genericstream.h"
#include "transportstream.h"
#include "compressmode.h"
#include "FileConfigurator.h"
#include "localconfigurator.h"
#include "HttpFileTransfer.h"
#include "configwrapper.h"
#include "inmalertdefs.h"

#include "setpermissions.h"

using namespace BOOST_SPIRIT_CLASSIC_NS;

void CreateTransportStream(boost::shared_ptr<TransportStream> &ts, const TRANSPORT_PROTOCOL &tp,
                           const TRANSPORT_CONNECTION_SETTINGS &tss, const VOLUME_SETTINGS::SECURE_MODE &securemode);

DisksLayoutUpdaterImp::DisksLayoutUpdaterImp(Configurator* configurator)
 : m_Configurator(configurator),
   m_IsUploading(m_Configurator->getVxAgent().getSerializerType() != Xmlize)
{
}


DisksLayoutUpdaterImp::~DisksLayoutUpdaterImp()
{
}


void DisksLayoutUpdaterImp::Update(const std::string &diskslayoutoption, Options_t & statusmap)
{
	UpdateDiskLabels_t udls[] = {&DisksLayoutUpdaterImp::DummyUpdateDiskLabels, &DisksLayoutUpdaterImp::UpdateDiskLabels};
	UpdateDiskLabels_t uld = udls[m_Configurator->getVxAgent().registerLabelOnDisks()];

	(this->*uld)(diskslayoutoption, statusmap);
}


void DisksLayoutUpdaterImp::Clear(const Options_t & statusmap)
{
    Clear_t clearFunctions[] = {&DisksLayoutUpdaterImp::ClearInNoUpload, &DisksLayoutUpdaterImp::ClearInUpload};
	Clear_t clearFunction = clearFunctions[m_IsUploading];

	(this->*clearFunction)(statusmap);
}


void DisksLayoutUpdaterImp::DummyUpdateDiskLabels(const std::string &diskslayoutoption, Options_t & statusmap)
{
  DebugPrintf( SV_LOG_DEBUG, "%s called\n", FUNCTION_NAME);
}


void DisksLayoutUpdaterImp::UpdateDiskLabels(const std::string &diskslayoutoption, Options_t & statusmap)
{
  DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
  std::stringstream labelFileNamePath;
  std::string labelInfoDirName, labelFileName;
  GetDiskLayoutFilePath_t pathFunctions[] = {&DisksLayoutUpdaterImp::GetDiskLayoutFilePathInNoUpload, &DisksLayoutUpdaterImp::GetDiskLayoutFilePathInUpload};
  GetDiskLayoutFilePath_t pathFunction = pathFunctions[m_IsUploading];
  DeleteOldDisksLayoutFiles_t delFunctions[] = {&DisksLayoutUpdaterImp::DeleteOldDisksLayoutFilesInNoUpload, &DisksLayoutUpdaterImp::DeleteOldDisksLayoutFilesInUpload};
  DeleteOldDisksLayoutFiles_t delFunction = delFunctions[m_IsUploading];
  ProcessDisksLayoutFiles_t createdFunctions[] = {&DisksLayoutUpdaterImp::ProcessCreatedFilesInNoUpload, &DisksLayoutUpdaterImp::ProcessCreatedFilesInUpload};
  ProcessDisksLayoutFiles_t createdFunction = createdFunctions[m_IsUploading];
  ProcessDisksLayoutFiles_t notCreatedFunctions[] = {&DisksLayoutUpdaterImp::ProcessNotCreatedFilesInNoUpload, &DisksLayoutUpdaterImp::ProcessNotCreatedFilesInUpload};
  ProcessDisksLayoutFiles_t notCreatedFunction = notCreatedFunctions[m_IsUploading];

  (this->*pathFunction)(labelFileNamePath);
  securitylib::setPermissions(labelFileNamePath.str(), securitylib::SET_PERMISSIONS_NAME_IS_DIR);
  labelInfoDirName = labelFileNamePath.str();
  DebugPrintf(SV_LOG_DEBUG, "disks label file directory is %s\n", labelInfoDirName.c_str());
  labelFileName = getDiskLayoutFileNameWOPath();
  DebugPrintf(SV_LOG_DEBUG, "disks label base file name is %s\n", labelFileName.c_str());
  labelFileNamePath << char(ACE_DIRECTORY_SEPARATOR_CHAR) << labelFileName;
  DebugPrintf(SV_LOG_DEBUG, "disks layout file name formed is %s, diskslayoutoption is %s\n", labelFileNamePath.str().c_str(),
	                        diskslayoutoption.c_str());
  std::list<std::string> erraticVolumeGuids ;  
  std::list<std::string> outFileNames ;
  DLM_ERROR_CODE ec = StoreDisksInfo(labelFileNamePath.str(), outFileNames, erraticVolumeGuids, diskslayoutoption);
  DebugPrintf(SV_LOG_DEBUG, "StoreDisksInfo returned %d for disks layout file %s\n", ec, labelFileNamePath.str().c_str());
  DebugPrintf(SV_LOG_DEBUG, "erraticVolumeGuids:\n");
  for_each(erraticVolumeGuids.begin(), erraticVolumeGuids.end(), PrintString);
  DebugPrintf(SV_LOG_DEBUG, "outFileNames:\n");
  for_each(outFileNames.begin(), outFileNames.end(), PrintString);

  /* TODO: handle labelandcreatefiles being of size zero ? 
   * Currently trust the api for this */
  if(DLM_FILE_CREATED == ec) {
	  DebugPrintf(SV_LOG_DEBUG, "New disk layout file(s) have been created by disk layout manager\n");
      AlertCxToErraticVolumes(erraticVolumeGuids);
	  (this->*delFunction)(outFileNames, labelInfoDirName);
      (this->*createdFunction)(labelFileName, outFileNames, statusmap);
  } else if (DLM_FILE_NOT_CREATED == ec) {
	  DebugPrintf(SV_LOG_DEBUG, "New disk layout file(s) are not created by disk layout manager\n");
      AlertCxToErraticVolumes(erraticVolumeGuids);
	  (this->*notCreatedFunction)(labelFileName, outFileNames, statusmap);
  } else {
	  DebugPrintf(SV_LOG_ERROR, "Getting %s from disk layout manager failed with DLM_ERROR_CODE: %d."
                                " Setting the status as failure to CS\n", labelFileNamePath.str().c_str(), ec);
	  statusmap.insert(std::make_pair(NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_KEY, labelFileName+std::string(1, NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_SEP)+NSRegisterHostStaticInfo::FAILURE));
  }

  DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}


void DisksLayoutUpdaterImp::GetDiskLayoutFilePathInNoUpload(std::stringstream & labelFileNamePath)
{
	FileConfigurator lc ;
	labelFileNamePath << lc.getRepositoryLocation() ;
	if( labelFileNamePath.str().length() == 1 )
	{
		labelFileNamePath << ":" ;
	}
	labelFileNamePath << ACE_DIRECTORY_SEPARATOR_CHAR_A ;
	labelFileNamePath << LABELFILESDIR;
	labelFileNamePath << ACE_DIRECTORY_SEPARATOR_CHAR_A  ;
	labelFileNamePath << m_Configurator->getVxAgent().getHostId();
}


void DisksLayoutUpdaterImp::GetDiskLayoutFilePathInUpload(std::stringstream & labelFileNamePath)
{
    labelFileNamePath << m_Configurator->getVxAgent().getCacheDirectory() << char(ACE_DIRECTORY_SEPARATOR_CHAR)
                      << LABELFILESDIR << char(ACE_DIRECTORY_SEPARATOR_CHAR) << m_Configurator->getVxAgent().getHostId();
}


std::string DisksLayoutUpdaterImp::getDiskLayoutFileNameWOPath(void)
{
  DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
  int iStatus = SV_SUCCESS;
  std::string strDriverTime;
  std::stringstream diskLayoutFile;

  do
  {
	diskLayoutFile << LABELFILENAMEPREFIX;

	//Look for disk filter and then volume filter
	DeviceStream *p = 0;
	DeviceStream volumeFilterStream(Device(INMAGE_FILTER_DEVICE_NAME));
	DeviceStream diskFilterStream(Device(INMAGE_DISK_FILTER_DOS_DEVICE_NAME));
	//If disk filter driver opens, get time stamp from it; else use volume filter driver
	if (SV_SUCCESS == diskFilterStream.Open(DeviceStream::Mode_Open | DeviceStream::Mode_RW | DeviceStream::Mode_ShareRW)) {
		DebugPrintf(SV_LOG_INFO, "Opened disk filter driver.\n");
		p = &diskFilterStream;
	}
	else if (SV_SUCCESS == volumeFilterStream.Open(DeviceStream::Mode_Open | DeviceStream::Mode_RW | DeviceStream::Mode_ShareRW)) {
		DebugPrintf(SV_LOG_INFO, "Opened volume filter driver.\n");
		p = &volumeFilterStream;
	}
	else {
		//Use system time if drivers are not present.
		DebugPrintf(SV_LOG_ERROR, "Failed to open disk filter driver %s with error %d and failed to open volume filter %s driver with error %d. Hence using system time.\n",
			diskFilterStream.GetName().c_str(), diskFilterStream.GetErrorCode(), volumeFilterStream.GetName().c_str(), volumeFilterStream.GetErrorCode());
		diskLayoutFile << getSystemTimeInUtc();
		break;
	}

    DRIVER_GLOBAL_TIMESTAMP driverTime;
    FilterDrvIfMajor drvif;
    iStatus = drvif.getDriverTime(*p, &driverTime);
	
    if ( SV_SUCCESS == iStatus)
    {
	  //DebugPrintf( SV_LOG_DEBUG, "DriverTime %llu SeqNo %llu\n", driverTime.TimeInHundNanoSecondsFromJan1601, driverTime.ullSequenceNumber);
      ConvertTimeToString(driverTime.TimeInHundNanoSecondsFromJan1601, strDriverTime);
	  diskLayoutFile << strDriverTime;
	  diskLayoutFile << "_" << driverTime.ullSequenceNumber;
      break;
    }
    else if ( ERR_INVALID_FUNCTION == iStatus)
    {
      DebugPrintf(SV_LOG_ERROR,"IOCTL_INMAGE_GET_GLOBAL_TIMESTAMP not supported, using system time\n");
      diskLayoutFile << getSystemTimeInUtc();
      break;
    }
	DebugPrintf( SV_LOG_DEBUG, "IOCTL_INMAGE_GET_GLOBAL_TIMESTAMP failed: %d\n", iStatus);
	// else don't use any time string
  } while (FALSE);
  DebugPrintf( SV_LOG_DEBUG, "diskLayoutFile %s\n", diskLayoutFile.str().c_str());
  DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
  return diskLayoutFile.str();
}


void DisksLayoutUpdaterImp::AlertCxToErraticVolumes(const std::list<std::string> & erraticVolumeGuids)
{
  std::list<std::string>::const_iterator erraticVolGuidIter ;
  erraticVolGuidIter = erraticVolumeGuids.begin() ;
  std::string guids;
  const char SEP = ',';
  while( erraticVolGuidIter != erraticVolumeGuids.end() ) {
    guids += *erraticVolGuidIter;
    guids += SEP;
    erraticVolGuidIter++ ;
  }
  if (!erraticVolumeGuids.empty()) {
    guids.erase(guids.end()-1);
    ErraticVolumesGUIDAlert a(guids);
    SendAlertToCx(SV_ALERT_ERROR, SV_ALERT_MODULE_HOST_REGISTRATION, a);
  }
}


void DisksLayoutUpdaterImp::DeleteOldDisksLayoutFilesInNoUpload(const std::list<std::string> &latestfiles, const std::string &directory)
{
	return;
}


void DisksLayoutUpdaterImp::DeleteOldDisksLayoutFilesInUpload(const std::list<std::string> &latestfiles, const std::string &directory)
{
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	std::string errmsg;
	std::string fileindirectory, readdirentry;
    ACE_DIR * ace_dir = NULL ;
    ACE_DIRENT * dirent = NULL ;
    ace_dir = sv_opendir(directory.c_str()) ;
	if( ace_dir ) {
		for(dirent = ACE_OS::readdir(ace_dir); dirent != NULL; dirent = ACE_OS::readdir(ace_dir)) {
			readdirentry = ACE_TEXT_ALWAYS_CHAR(dirent->d_name);
			if ((readdirentry == ".") || (readdirentry == ".."))
				continue;
			fileindirectory = directory + ACE_DIRECTORY_SEPARATOR_CHAR_A + readdirentry;
			/* TODO: write a generic operation on file function with respect to containers */
			if (latestfiles.end() == std::find(latestfiles.begin(), latestfiles.end(), fileindirectory)) {
				if (InmRemoveFile(fileindirectory, errmsg))
					DebugPrintf(SV_LOG_DEBUG, "Removed older disks layout file %s successfully\n", fileindirectory.c_str());
				else
					DebugPrintf(SV_LOG_ERROR, "Removing old disks layout file file %s failed with error: %s\n", fileindirectory.c_str(), errmsg.c_str());
			}
		}
		ACE_OS::closedir( ace_dir ) ;
	} else
		DebugPrintf(SV_LOG_ERROR, "opening directory %s failed with error number %d\n", directory.c_str(), ACE_OS::last_error());

	DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}


void DisksLayoutUpdaterImp::ProcessCreatedFilesInNoUpload(const std::string &labelfile,
                                                          const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap)
{
	return;
}


void DisksLayoutUpdaterImp::ProcessCreatedFilesInUpload(const std::string &labelfile,
                                                        const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	SetExistingFiles(labelfileandrelatedfiles);
    std::vector<std::string>::size_type nfilesexisting = m_ExistingFiles.size();

	if (nfilesexisting == 0) {
		DebugPrintf(SV_LOG_ERROR, "Disk layout manager has not created any label file, out of the said \"created\", for file %s."
                                  " Setting the status as failure to CS\n", labelfile.c_str());
		statusmap.insert(std::make_pair(NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_KEY, labelfile+std::string(1, NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_SEP)+NSRegisterHostStaticInfo::FAILURE));
	} else if (nfilesexisting < labelfileandrelatedfiles.size()) {
		DebugPrintf(SV_LOG_DEBUG, "Disk layout manager has created less number of files, out of the said \"created\", for file %s."
			" Considering this as case where label file has reference to already sent file(s). Uploading the existing ones.\n", labelfile.c_str());
        UploadExistingFiles(labelfile, statusmap);
	} else
		UploadExistingFiles(labelfile, statusmap);

	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}


void DisksLayoutUpdaterImp::ProcessNotCreatedFilesInNoUpload(const std::string &labelfile, const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap)
{
	return;
}


void DisksLayoutUpdaterImp::ProcessNotCreatedFilesInUpload(const std::string &labelfile, const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	SetExistingFiles(labelfileandrelatedfiles);
    std::vector<std::string>::size_type nfilesexisting = m_ExistingFiles.size();
	if (nfilesexisting == 0) {
        DebugPrintf(SV_LOG_DEBUG, "Disk layout manager has not created new label files for file %s. "
                                  "Considering these as already uploaded earlier.\n", labelfile.c_str());
	} else {
		DebugPrintf(SV_LOG_DEBUG, "Although disks layout manager said, \"no files created\", "
                                  "for disks layout file %s, all or some previous files are present. "
                                  "Hence uploading these, considering that these were not, or partially, uploaded (or) "
                                  "local deletion failed earlier after upload(NOTE: In this case, this is overwrite).\n", labelfile.c_str());
		UploadExistingFiles(labelfile, statusmap);
	}

	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}


void DisksLayoutUpdaterImp::SetExistingFiles(const std::list<std::string> &labelfileandrelatedfiles)
{
	m_ExistingFiles = for_each(labelfileandrelatedfiles.begin(), labelfileandrelatedfiles.end(), CollectExistingFiles());
	std::vector<std::string>::size_type nfilesexisting = m_ExistingFiles.size();
	DebugPrintf(SV_LOG_DEBUG, "Below are the existing files out of the outFileNames:\n");
	for (std::vector<std::string>::size_type i = 0; i < nfilesexisting; i++) {
		DebugPrintf(SV_LOG_DEBUG, "%s\n", m_ExistingFiles[i].c_str());
		const std::string &baseexistingname = basename_r(m_ExistingFiles[i].c_str());
		if (IsLabelFileForStatus(baseexistingname)) {
			m_LabelFileForStatus = baseexistingname;
			DebugPrintf(SV_LOG_DEBUG, "Label file to send status is %s\n", m_LabelFileForStatus.c_str());
		}
	}
}


void DisksLayoutUpdaterImp::DeleteExistingFiles(const SV_LOG_LEVEL &loglevel)
{
	std::string errmsg;

	std::vector<std::string>::size_type nfilesexisting = m_ExistingFiles.size(); 
	for (std::vector<std::string>::size_type i = 0; i < nfilesexisting; i++) {
		const std::string &existingfile = m_ExistingFiles[i];
		if (InmRemoveFile(existingfile, errmsg))
			DebugPrintf(loglevel, "Deleted %s successfully, as this was an uploaded disks layout file.\n", existingfile.c_str());
		else
			DebugPrintf(SV_LOG_ERROR, "Failed to delete an uploaded disks layout file %s with error %s\n", existingfile.c_str(), errmsg.c_str());
	}
}


void DisksLayoutUpdaterImp::UploadExistingFiles(const std::string &labelfile, Options_t &statusmap)
{
	FileUpload(labelfile, statusmap);
}


void DisksLayoutUpdaterImp::FileUpload(const std::string &labelfile, Options_t &statusmap)
{
	DebugPrintf(SV_LOG_DEBUG,"Entered %s\n",__FUNCTION__);
	
	bool res = true ;
	std::string  labelremotedir;
	const char *status = NSRegisterHostStaticInfo::SUCCESS;
	LocalConfigurator lc ;
	bool bHttps = lc.IsHttps();

	//Find CX Ip from Local Configurtaor
	HTTP_CONNECTION_SETTINGS s = m_Configurator->getHttpSettings();
	std::string strCxIpAddress(s.ipAddress)  ;
   
	//Find CX port number
	std::string  httpPortNumber =  boost::lexical_cast<std::string>(s.port) ;
	
	boost::shared_ptr<FileTransfer> ftPtr(new HttpFileTransfer(bHttps,strCxIpAddress,httpPortNumber));
	if(ftPtr->Init())
	{
		labelremotedir = LABELFILESREMOTEDIR + LABELFILESDIR + '/' + m_Configurator->getVxAgent().getHostId() ;
			std::vector<std::string>::size_type nfilesexisting = m_ExistingFiles.size(); 

			for (std::vector<std::string>::size_type i = 0; i < nfilesexisting; i++) 
			{
				const std::string &currentPath = m_ExistingFiles[i];
				DebugPrintf(SV_LOG_DEBUG, "sending label file %s, in remote directory %s\n",
					currentPath.c_str(), labelremotedir.c_str());
				if ( !ftPtr->Upload( labelremotedir,currentPath ) )
				{
					res = false ;
                    DlmFileUploadFailedAlert a(currentPath, labelremotedir, strCxIpAddress, s.port, bHttps);
					DebugPrintf(SV_LOG_ERROR, "%s\n", a.GetMessage().c_str());
                    SendAlertToCx(SV_ALERT_CRITICAL, SV_ALERT_MODULE_HOST_REGISTRATION, a);
					break ;
				}
				else
				{
					DebugPrintf(SV_LOG_DEBUG, "successfully sent label file %s, in remote directory %s\n",
						currentPath.c_str(), labelremotedir.c_str());
				}
			}
		
	}
	if(!res)
	{
		status = NSRegisterHostStaticInfo::FAILURE;
	}
	
	if (m_LabelFileForStatus.empty()) 
		DebugPrintf(SV_LOG_ERROR, "Could not find label file to send status to CS for label file %s\n", labelfile.c_str());
	else
		statusmap.insert(std::make_pair(NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_KEY, m_LabelFileForStatus+std::string(1, NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_SEP)+status));
	 DebugPrintf(SV_LOG_DEBUG,"Exiting %s\n",__FUNCTION__);
}
void DisksLayoutUpdaterImp::ClearInNoUpload(const Options_t & statusmap)
{
	return;
}


void DisksLayoutUpdaterImp::ClearInUpload(const Options_t & statusmap)
{
	DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	Options_t::const_iterator it = statusmap.find(NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_KEY);
	if (it != statusmap.end()) {
		const std::string &value = it->second;
		DebugPrintf(SV_LOG_DEBUG, "While clearing disks layout files, found disks layout status key %s with value %s\n", 
			NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_KEY, value.c_str());
		ClearIfFilesWereUploaded(value);
	} else {
		DebugPrintf(SV_LOG_DEBUG, "While clearing disks layout files, did not find disks layout status key %s."
			" There were no uploads.\n", NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_KEY);
	}

	DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}


void DisksLayoutUpdaterImp::ClearIfFilesWereUploaded(const std::string &filewithstatus)
{
	char sep;
	std::string revfilename, revstatus;
	const std::string s(NSRegisterHostStaticInfo::SUCCESS);
	const std::string f(NSRegisterHostStaticInfo::FAILURE);

	strlit<std::string::const_reverse_iterator> revsuccess(s.rbegin(), s.rend());
	strlit<std::string::const_reverse_iterator> revfailure(f.rbegin(), f.rend());
	bool parsed = parse(filewithstatus.rbegin(), filewithstatus.rend(), 
		lexeme_d
		[
			(revsuccess | revfailure) [phoenix::var(revstatus) = phoenix::construct_<std::string>(phoenix::arg1,phoenix::arg2)]
			>> ch_p(NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_SEP) [phoenix::var(sep) = phoenix::arg1]
			>> (+anychar_p) [phoenix::var(revfilename) = phoenix::construct_<std::string>(phoenix::arg1,phoenix::arg2)]
		] >> end_p,
		space_p).full;

	if (parsed) {
		std::string filename(revfilename.rbegin(), revfilename.rend());
		std::string status(revstatus.rbegin(), revstatus.rend());
		DebugPrintf(SV_LOG_DEBUG, "for cleanup, disks label file name is %s, upload status %s\n", filename.c_str(), status.c_str());
		if (s == status) {
            DebugPrintf(SV_LOG_DEBUG, "Deleting label files as these were uploaded successfully\n");
			DeleteExistingFiles(SV_LOG_DEBUG);
		} else
			DebugPrintf(SV_LOG_DEBUG, "Not deleting label files as these were not uploaded successfully\n");
	} else
		DebugPrintf(SV_LOG_ERROR, "The status value %s for key %s is invalid\n", filewithstatus.c_str(), NSRegisterHostStaticInfo::DISKS_LAYOUT_STATUS_KEY);
}


bool DisksLayoutUpdaterImp::IsLabelFileForStatus(const std::string &file)
{
	strlit<std::string::const_iterator> prefixrule(LABELFILENAMEPREFIX.begin(), LABELFILENAMEPREFIX.end());

	/* older driver does not support ioctl to give time 
	 * and sequence number. Hence the disks layout file 
	 * name in this case will contain system time and not
	 * sequence number */
	return parse(file.begin(), file.end(), 
                 lexeme_d [
                           prefixrule
                           >> +digit_p
						   >> !(ch_p('_') >> +digit_p)
                          ] >> end_p, 
                 space_p).full;
}
