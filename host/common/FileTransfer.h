#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

class FileTransfer
{
public:
	virtual bool Init() = 0;
	/* Upload File from a local path to a remote directory */
    virtual bool Upload(const std::string& remotedir,const std::string& localFilePath) = 0;
	/* Downlod File from a remote path to local */
    virtual bool Download(const std::string& remoteFilePath,const std::string& localFilePath) = 0;

	virtual ~FileTransfer() { }
};

#endif /* FILE_TRANSFER_H */