#ifndef SECOPS_H_
#define SECOPS_H_

#include <string>

class SecurityOps {
		std::string &packetData;
	public:
		SecurityOps(std::string &&data) : packetData(data) {};
		bool verifyCRC(uint32_t crcChecksum);
		bool verfiyEncryption();
};

#endif
