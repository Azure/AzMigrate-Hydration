#ifndef DISKS_LAYOUT_UPDATER_IMP_H
#define DISKS_LAYOUT_UPDATER_IMP_H

#include <sstream>
#include <list>
#include "diskslayoutupdater.h"
#include "configurator2.h"

class DisksLayoutUpdaterImp : public DisksLayoutUpdater
{
public:
	DisksLayoutUpdaterImp(Configurator *configurator);
	~DisksLayoutUpdaterImp();
    void Update(const std::string &diskslayoutoption, Options_t & statusmap);
    void Clear(const Options_t & statusmap);

private:
	/* TODO: some functions have to be made common to all platforms */
	typedef void (DisksLayoutUpdaterImp::*UpdateDiskLabels_t)(const std::string &diskslayoutoption, Options_t & statusmap);
	typedef void (DisksLayoutUpdaterImp::*GetDiskLayoutFilePath_t)(std::stringstream & labelFileNamePath);
	typedef void (DisksLayoutUpdaterImp::*DeleteOldDisksLayoutFiles_t)(const std::list<std::string> &latestfiles, const std::string &directory);
	typedef void (DisksLayoutUpdaterImp::*ProcessDisksLayoutFiles_t)(const std::string &labelfile, const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap);
	typedef void (DisksLayoutUpdaterImp::*Clear_t)(const Options_t & statusmap);

	void UpdateDiskLabels(const std::string &diskslayoutoption, Options_t & statusmap);
	void DummyUpdateDiskLabels(const std::string &diskslayoutoption, Options_t & statusmap);

	void GetDiskLayoutFilePathInNoUpload(std::stringstream & labelFileNamePath);
	void GetDiskLayoutFilePathInUpload(std::stringstream & labelFileNamePath);

	void DeleteOldDisksLayoutFilesInNoUpload(const std::list<std::string> &latestfiles, const std::string &directory);
	void DeleteOldDisksLayoutFilesInUpload(const std::list<std::string> &latestfiles, const std::string &directory);

	void ProcessCreatedFilesInNoUpload(const std::string &labelfile, const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap);
	void ProcessCreatedFilesInUpload(const std::string &labelfile, const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap);

	void ProcessNotCreatedFilesInNoUpload(const std::string &labelfile, const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap);
	void ProcessNotCreatedFilesInUpload(const std::string &labelfile, const std::list<std::string> &labelfileandrelatedfiles, Options_t &statusmap);

	void ClearInNoUpload(const Options_t & statusmap);
	void ClearInUpload(const Options_t & statusmap);

	std::string getDiskLayoutFileNameWOPath(void);
	void AlertCxToErraticVolumes(const std::list<std::string> & erraticVolumeGuids);
	void DeleteExistingFiles(const SV_LOG_LEVEL &loglevel = SV_LOG_DEBUG);
	void UploadExistingFiles(const std::string &labelfile, Options_t &statusmap);
	void FileUpload(const std::string &labelfile, Options_t &statusmap);
	void ClearIfFilesWereUploaded(const std::string &filewithstatus);
	void SetExistingFiles(const std::list<std::string> &labelfileandrelatedfiles);
	bool IsLabelFileForStatus(const std::string &file);

private:
	Configurator *m_Configurator;
	bool m_IsUploading;
	CollectExistingFiles m_ExistingFiles;
	std::string m_LabelFileForStatus;
};

#endif /* DISKS_LAYOUT_UPDATER_IMP_H */
