#include "cxps.h"
#include "cxpslogger.h"

#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <handleapi.h>
#include <ktmw32.h>
#include <Shlwapi.h>

#define LOG_ERROR(error, msg) CXPS_LOG_ERROR(AT_LOC << "Failed to " << msg << " with error : " << error)
#define LOG_LAST_ERROR(msg) LOG_ERROR(GetLastError(), msg)

#define LOG_AND_THROW_ON_ERROR(error, msg) \
    do { \
        std::wstringstream errSS; \
        errSS << "Failed to " << msg << " with error : " << error; \
        auto errStr = convertWstringToUtf8(errSS.str()); \
        CXPS_LOG_ERROR(THROW_LOC << errStr); \
        throw std::runtime_error(errStr); \
    } while(false)

#define LOG_AND_THROW_ON_LAST_ERROR(msg) LOG_AND_THROW_ON_ERROR(GetLastError(), msg)

// Copying this method to avoid linking common project in server.sln
static std::string convertWstringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string("");

    const int reqSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strResult(reqSize, 0);
    int ret = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&wstr[0], (int)wstr.length(), (LPSTR)&strResult[0], strResult.size(), NULL, NULL);
    if (ret == 0)
    {
        DWORD err = GetLastError();
        std::stringstream sserr;
        sserr << "convertWstringToUtf8: WideCharToMultiByte Failed. Error: " << err;
        throw std::exception(sserr.str().c_str());
    }

    return strResult;
}

static void RegCloseKeyWrapper(HKEY hkey, const std::wstring& path)
{
    // Simplest fix to use boost::bind on __stdcall
    LSTATUS status = RegCloseKey(hkey);
    if (status != ERROR_SUCCESS) {
        LOG_ERROR(status, "close registry key : " << path << " using handle : " << hkey);
    }
}

static void TransactionRollbackWrapper(HANDLE handle, const std::wstring& transactionName)
{
    // Simplest fix to use boost::bind on __stdcall
    if (!RollbackTransaction(handle)) {
        LOG_LAST_ERROR(
            "rollback transaction : " << transactionName << " using handle : " << handle);
    }
}

static void CloseHandleWrapper(HANDLE handle, const std::wstring& objectPath)
{
    // Simplest fix to use boost::bind on __stdcall
    if (!CloseHandle(handle)) {
        LOG_LAST_ERROR("close handle : " << handle << " of object : " << objectPath);
    }
}

static uint64_t GetDotNetUtcTicks()
{
    // DateTime objects in .Net start from 01.01.0001 but the valid gregorian
    // date range in boost starts from year 1400.
    auto durSince1400 =
        boost::posix_time::microsec_clock::universal_time() -
        boost::posix_time::ptime(boost::gregorian::date(1400, 1, 1));

    const uint64_t ticksTill1400 = 441481536000000000; // ticks from 0001-01-01 to 1400-01-01
    return durSince1400.total_microseconds() * 10 + ticksTill1400;
}

static std::wstring BuildBackupFilePath(const std::wstring& filePath, const std::wstring& prefix, bool isPrev, const std::wstring& backupFolderPath)
{
    boost::filesystem::path inputPath(filePath);
    boost::filesystem::path resultPath =
        backupFolderPath.empty() ? inputPath.parent_path().wstring() : backupFolderPath;

    if (!boost::filesystem::exists(resultPath)) {
        // TODO-SanKumar-2006: This is not as strict as FSUtils.CreateAdminsAndSystemOnlyDir
        boost::filesystem::create_directories(resultPath);
    }

    resultPath /= (prefix.empty()? std::wstring() : prefix) + inputPath.stem().wstring();
    resultPath += "_bak_";
    resultPath += isPrev ? "prev" : "curr";
    resultPath += inputPath.extension();

    return resultPath.wstring();
}

static std::wstring ReadExpandableStringValue(
    HKEY hkey,
    const std::wstring& subkeyName,
    const std::wstring& valueName,
    bool ignoreIfNotFound = false)
{
    DWORD valueBytes = 0;
    DWORD valueType;

    LSTATUS status = RegGetValueW(
        hkey,
        subkeyName.c_str(),
        valueName.c_str(),
        RRF_RT_ANY,
        &valueType,
        NULL,
        &valueBytes);

    if (ignoreIfNotFound && status == ERROR_FILE_NOT_FOUND) {
        return std::wstring();
    }

    if (status != ERROR_SUCCESS) {
        LOG_AND_THROW_ON_ERROR(status,
            "get size of value : " << valueName << " in subkey : " << subkeyName);
    }

    if (valueType != REG_SZ && valueType != REG_EXPAND_SZ) {
        LOG_AND_THROW_ON_ERROR(ERROR_UNSUPPORTED_TYPE,
            "unexpected type : " << valueType << " instead of REG_SZ : "<< REG_SZ
            << " or REG_EXPAND_SZ : " << REG_EXPAND_SZ);
    }

    std::vector<BYTE> valueBuff(valueBytes);

    status = RegGetValueW(
        hkey,
        subkeyName.c_str(),
        valueName.c_str(),
        RRF_RT_ANY,
        &valueType,
        valueBuff.data(),
        &valueBytes);

    if (status != ERROR_SUCCESS) {
        LOG_AND_THROW_ON_ERROR(status, "get value : " << valueName << " in subkey : " << subkeyName);
    }

    if (valueType != REG_SZ && valueType != REG_EXPAND_SZ) {
        LOG_AND_THROW_ON_ERROR(ERROR_UNSUPPORTED_TYPE,
            "unexpected type : " << valueType << " instead of REG_SZ : " << REG_SZ
            << " or REG_EXPAND_SZ : " << REG_EXPAND_SZ);
    }

    // The string buffer is guaranteed to be NULL terminated
    return std::wstring(reinterpret_cast<wchar_t*>(valueBuff.data()));
}

static void FlushFile(const std::wstring& filePath, bool throwOnFailure)
{
    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_WRITE,
        0, // Exclusive open
        NULL, // lpSecurityAttributes
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL); // hTemplateFile

    if (hFile == INVALID_HANDLE_VALUE) {
        if (throwOnFailure) {
            LOG_AND_THROW_ON_LAST_ERROR("get handle for file : " << filePath);
        }
        else {
            LOG_LAST_ERROR("get handle for file : " << filePath);
            return;
        }
    }

    auto fileCloseGuard = MAKE_SCOPE_GUARD(boost::bind(CloseHandleWrapper, hFile, filePath));

    if (!FlushFileBuffers(hFile)) {
        if (throwOnFailure) {
            LOG_AND_THROW_ON_LAST_ERROR("flush file : " << filePath << " using handle : " << hFile);
        }
        else {
            LOG_LAST_ERROR("flush file : " << filePath << " using handle : " << hFile);
            return;
        }
    }
}

static const boost::filesystem::path IDEMP_KEY_PATH(LR"(SOFTWARE\Microsoft\Azure Site Recovery Process Server\Idempotency)");

static void ExecutePerSubkey(HANDLE hTransaction, std::wstring subkeyParentName, boost::function<void(HKEY, const std::wstring&)> action)
{
    HKEY hkey;
    auto subKeyParentPath = (IDEMP_KEY_PATH / subkeyParentName).wstring();

    LSTATUS status = RegOpenKeyTransactedW(
        HKEY_LOCAL_MACHINE,
        subKeyParentPath.c_str(),
        0, // Reserved
        KEY_WOW64_64KEY | KEY_READ,
        &hkey,
        hTransaction,
        NULL); // pExtendedParemeter

    if (status == ERROR_FILE_NOT_FOUND) {
        return;
    }

    if (status != ERROR_SUCCESS) {
        LOG_AND_THROW_ON_ERROR(status, "open handle for subkey parent : " << subKeyParentPath);
    }

    SCOPE_GUARD regKeyGuard = MAKE_SCOPE_GUARD(boost::bind(RegCloseKeyWrapper, hkey, subKeyParentPath));

    const DWORD KEY_NAME_MAX_SIZE = 256; // Max reg key name length is 255 chars
    std::vector<wchar_t> keyNameVect(KEY_NAME_MAX_SIZE);

    for (;;) {
        DWORD currKeyNameSize = KEY_NAME_MAX_SIZE;

        LSTATUS status = RegEnumKeyExW(
            hkey,
            0, // dwIndex
            keyNameVect.data(),
            &currKeyNameSize,
            NULL, // lpReserved
            NULL, // lpClass
            NULL, // lpcchClass
            NULL); // lpftLastWriteTime

        if (status != ERROR_SUCCESS && status != ERROR_NO_MORE_ITEMS) {
            LOG_AND_THROW_ON_ERROR(status, "get next subkey name length for : " << subKeyParentPath);
        }

        if (status == ERROR_NO_MORE_ITEMS) {
            // All the keys have been enumerated and deleted
            break;
        }

        std::wstring currKeyName(keyNameVect.data(), currKeyNameSize);

        action(hkey, currKeyName);

        status = RegDeleteKeyTransactedW(
            hkey,
            currKeyName.c_str(),
            KEY_WOW64_64KEY,
            0, // Reserved
            hTransaction,
            NULL); // pExtendedParemeter

        if (status != ERROR_SUCCESS) {
            LOG_AND_THROW_ON_ERROR(status,
                "delete subkey : " << currKeyName << " of key : " << subKeyParentPath);
        }
    }

    regKeyGuard.dismiss();
    status = RegCloseKey(hkey);
    if (status != ERROR_SUCCESS) {
        LOG_AND_THROW_ON_ERROR(status, "close handle : " << hkey << " of key : " << subKeyParentPath);
    }
}

typedef std::vector<std::pair<std::wstring, std::wstring>> FileBackupFilesTuples;

static void HandleFilesToReplace(HANDLE hTransaction, FileBackupFilesTuples& postCommitMoveTuples)
{
    FileBackupFilesTuples localPostCommitMoveTuples;

    ExecutePerSubkey(hTransaction, L"FilesToReplace",
        [&localPostCommitMoveTuples](HKEY hkey, const std::wstring& subkeyName) {

        auto originalFilePath = ReadExpandableStringValue(hkey, subkeyName, L"Full Path", false);
        auto latestFilePath = ReadExpandableStringValue(hkey, subkeyName, L"Latest File Path", true);
        auto backupFolderPath = ReadExpandableStringValue(hkey, subkeyName, L"Backup Folder Path", true);
        auto lockFilePath = ReadExpandableStringValue(hkey, subkeyName, L"Lock File Path", true);

        if (originalFilePath.empty()) {
            throw std::logic_error("Full Path value can't be empty");
        }

        boost::interprocess::file_lock fileLock;
        boost::interprocess::scoped_lock<boost::interprocess::file_lock> fileLockGuard;
        if (!lockFilePath.empty())
        {
            if (!boost::filesystem::exists(lockFilePath)) {
                std::ofstream tmpLockFile(lockFilePath);
            }

            fileLock = boost::interprocess::file_lock(convertWstringToUtf8(lockFilePath).c_str());
            fileLockGuard = boost::interprocess::scoped_lock<boost::interprocess::file_lock>(fileLock);
        }

        auto prefix = std::to_wstring(GetDotNetUtcTicks()) + L'_';
        std::wstring backupFilePathPrev, backupFilePathCurr;
        backupFilePathPrev = BuildBackupFilePath(originalFilePath, prefix, true, backupFolderPath);
        backupFilePathCurr = BuildBackupFilePath(originalFilePath, prefix, false, backupFolderPath);

        // Backup original file
        if (boost::filesystem::exists(originalFilePath.c_str())) {
            if (!CopyFileW(originalFilePath.c_str(), backupFilePathPrev.c_str(), FALSE)) {
                LOG_AND_THROW_ON_LAST_ERROR(
                    "backup original file : " << originalFilePath << " to : " << backupFilePathPrev);
            }

            FlushFile(backupFilePathPrev, true);
        }

        if (latestFilePath.empty()) {
            // Delete the original file

            // TODO-SanKumar-2006: Should we do, handle open,
            // delete (marks for delete), flush and then close handle?
            // Would that ensure that the file deletion is completely
            // persisted to the file system?
            if (boost::filesystem::exists(originalFilePath)) {
                if (!DeleteFileW(originalFilePath.c_str())) {
                    LOG_AND_THROW_ON_LAST_ERROR("delete original file : " << originalFilePath);
                }
            }
        } else if (boost::filesystem::exists(latestFilePath)) {
            // Overwrite original file with the latest file
            if (!CopyFileW(latestFilePath.c_str(), originalFilePath.c_str(), FALSE)) {
                LOG_AND_THROW_ON_LAST_ERROR(
                    "overwrite original file : " << originalFilePath << " with : " << latestFilePath);
            }

            FlushFile(originalFilePath, true);

            localPostCommitMoveTuples.push_back(std::make_pair(latestFilePath, backupFilePathCurr));
        }
    });

    postCommitMoveTuples = localPostCommitMoveTuples;
}

static void HandleFoldersToDelete(HANDLE hTransaction)
{
    ExecutePerSubkey(hTransaction, L"FoldersToDelete",
        [](HKEY hkey, const std::wstring& subkeyName) {

        auto folderPath = ReadExpandableStringValue(hkey, subkeyName, L"Full Path", false);
        auto lockFilePath = ReadExpandableStringValue(hkey, subkeyName, L"Lock File Path", true);

        if (folderPath.empty()) {
            throw std::logic_error("Full Path value can't be empty");
        }

        boost::interprocess::file_lock fileLock;
        boost::interprocess::scoped_lock<boost::interprocess::file_lock> fileLockGuard;
        if (!lockFilePath.empty())
        {
            if (!boost::filesystem::exists(lockFilePath)) {
                std::ofstream tmpLockFile(lockFilePath);
            }

            fileLock = boost::interprocess::file_lock(convertWstringToUtf8(lockFilePath).c_str());
            fileLockGuard = boost::interprocess::scoped_lock<boost::interprocess::file_lock>(fileLock);
        }

        if (boost::filesystem::exists(folderPath)) {
            boost::filesystem::remove_all(folderPath);
        }
    });
}

// Entry point
void idempotentRestorePSState()
{
    // Acquire idempotency file lock to exclusively perform this operation across
    // native and managed processes
    std::wstring idempLckFilePath = GetRcmPSInstallationInfo().m_idempotencyLckFilePath;
    if (!boost::filesystem::exists(idempLckFilePath)) {
        std::ofstream tmpLockFile(idempLckFilePath.c_str());
    }
    boost::interprocess::file_lock idempFileLock(convertWstringToUtf8(idempLckFilePath).c_str());
    boost::interprocess::scoped_lock<boost::interprocess::file_lock> idempFileLockGuard(idempFileLock);

    // Create transaction
    const std::wstring TRANSACTION_NAME(L"IdempotentRestoreNativeTransaction");

    // TODO: Should we set this to some finite value?
    auto timeoutMS = 0; // INFINITE
    HANDLE hTransaction = CreateTransaction(
        NULL, // lpTransactionAttributes,
        0, // Reserved
        TRANSACTION_DO_NOT_PROMOTE,
        0, // Reserved
        0, // Reserved
        timeoutMS,
        const_cast<wchar_t*>(TRANSACTION_NAME.c_str()));

    if (hTransaction == INVALID_HANDLE_VALUE) {
        LOG_AND_THROW_ON_LAST_ERROR("create idempotency transaction : " << TRANSACTION_NAME);
    }

    SCOPE_GUARD transCloseGuard =
        MAKE_SCOPE_GUARD(boost::bind(CloseHandleWrapper, hTransaction, TRANSACTION_NAME));

    SCOPE_GUARD transRollbackGuard =
        MAKE_SCOPE_GUARD(boost::bind(TransactionRollbackWrapper, hTransaction, TRANSACTION_NAME));

    FileBackupFilesTuples postCommitMoveTuples;
    HandleFilesToReplace(hTransaction, postCommitMoveTuples);
    HandleFoldersToDelete(hTransaction);

    // Commit transaction
    if (!CommitTransaction(hTransaction)) {
        LOG_AND_THROW_ON_LAST_ERROR(
            "commit transaction : " << TRANSACTION_NAME << " using handle : " << hTransaction);
    }

    transRollbackGuard.dismiss();

    for (const auto currPair : postCommitMoveTuples) {

        if (!boost::filesystem::exists(currPair.first))
            continue;

        if (!MoveFileExW(currPair.first.c_str(), currPair.second.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            LOG_LAST_ERROR("backup latest file : " << currPair.first << " to : " << currPair.second);
        }
        else {
            FlushFile(currPair.second, false); // Doesn't throw
        }
    }
}
