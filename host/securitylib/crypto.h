#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>

namespace securitylib
{
    std::string systemEncrypt(const std::string &plainText);

    std::string systemDecrypt(const std::string &cipherText);

	std::string systemDecrypt(const std::string &cipherText, void* entropyBlob);
}

#endif // !CRYPTO_H
