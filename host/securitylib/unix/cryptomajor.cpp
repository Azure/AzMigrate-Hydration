#include "crypto.h"
#include <stdexcept>

namespace securitylib
{
    std::string systemEncrypt(const std::string &plainText)
	{
		throw std::runtime_error("Unsupported in Unix");
	}

    std::string systemDecrypt(const std::string &cipherText)
	{
		throw std::runtime_error("Unsupported in Unix");
	}
	
	std::string systemDecrypt(const std::string &cipherText, void* entropyBlob)
	{
		throw std::runtime_error("Unsupported in Unix");
	}
}
