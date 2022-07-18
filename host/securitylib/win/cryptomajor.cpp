#include <windows.h>
#include <dpapi.h>

#include <boost/bind.hpp>
#include <boost/system/system_error.hpp>

#include "crypto.h"
#include "scopeguard.h"

namespace securitylib
{
    static HLOCAL LocalFreeWrapper(HLOCAL memory)
    {
        // Simplest fix to use boost::bind on __stdcall
        return LocalFree(memory);
    }

    std::string systemEncrypt(const std::string &plainText)
    {
        DATA_BLOB plainBlob, cipherBlob;

        plainBlob.pbData = (BYTE*)plainText.c_str();
        plainBlob.cbData = (DWORD)plainText.size();

        BOOL result = CryptProtectData(
            &plainBlob,
            NULL,
            NULL,
            NULL,
            NULL,
            CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN,
            &cipherBlob);

        if (!result)
        {
            throw boost::system::system_error(GetLastError(), boost::system::system_category());
        }

        SCOPE_GUARD cipherBlobGuard = MAKE_SCOPE_GUARD(boost::bind(LocalFreeWrapper, cipherBlob.pbData));

        return std::string((const char*)cipherBlob.pbData, cipherBlob.cbData);
    }

	std::string systemDecrypt(const std::string &cipherText, void* entropyBlob)
	{
		std::string plainText;
		DATA_BLOB cipherBlob, plainBlob;

		cipherBlob.pbData = (BYTE*)cipherText.c_str();
		cipherBlob.cbData = (DWORD)cipherText.size();

		BOOL result = CryptUnprotectData(
			&cipherBlob,
			NULL,
			(DATA_BLOB*)entropyBlob,
			NULL,
			NULL,
			CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN,
			&plainBlob);

		// TODO-SanKumar-1909 : Implement CRYPTPROTECT_VERIFY_PROTECTION - hint with bool&
		// This flag verifies the protection of a protected BLOB.If the default protection
		// level configured of the host is higher than the current protection level for
		// the BLOB, the function returns CRYPT_I_NEW_PROTECTION_REQUIRED to advise the
		// caller to again protect the plaintext contained in the BLOB.

		if (!result)
		{
			throw boost::system::system_error(GetLastError(), boost::system::system_category());
		}

		SCOPE_GUARD plainBlobGuard = MAKE_SCOPE_GUARD(boost::bind(LocalFreeWrapper, plainBlob.pbData));

		return std::string((const char*)plainBlob.pbData, plainBlob.cbData);
	}

    std::string systemDecrypt(const std::string &cipherText)
    {
		return systemDecrypt(cipherText, NULL);
	}
}
