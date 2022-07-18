
///
/// \file importpfxcert.h
///
/// \brief
///

#ifndef IMPORTPFXCERT_H
#define IMPORTPFXCERT_H

#include <windows.h>
#include <Cryptuiapi.h>
#include <string>

namespace securitylib {
    

    inline bool importPfxCert(std::string const& pfxName, std::string const& passphrase)
    {
        CRYPTUI_WIZ_IMPORT_SRC_INFO importSrc;
        memset(&importSrc, 0, sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO));
        importSrc.dwSize = sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
        importSrc.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
        std::wstring widePfx(pfxName.begin(), pfxName.end());
        importSrc.pwszFileName = widePfx.c_str();
        std::wstring widePassphrase(passphrase.begin(), passphrase.end());
        importSrc.pwszPassword = widePassphrase.c_str();
        importSrc.dwFlags = CRYPT_EXPORTABLE | CRYPT_MACHINE_KEYSET;
        if (0 == CryptUIWizImport(CRYPTUI_WIZ_NO_UI | CRYPTUI_WIZ_IMPORT_TO_LOCALMACHINE, 0, 0, &importSrc, 0)) {
            std::cerr << "Error: import cert to local store failed: " << GetLastError();
            return false;
        }
        return true;
    }

} // namespace securitylib

#endif // IMPORTPFXCERT_H
