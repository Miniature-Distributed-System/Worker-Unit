#include "security_ops.hpp"
#include "zlib.h"
#include "../include/logger.hpp"

std::string decimalToHex(int decimalNumber)
{
	std::stringstream hexStream;
	hexStream << std::hex << std::uppercase << decimalNumber;
}

uint32_t hexToDecimal(const std::string &hexString)
{
	std::stringstream decimalStream;
	decimalStream << std::hex << hexString;
	uint32_t decimalNumber;
	decimalStream >> decimalNumber;
	return decimalNumber; 
}

bool SecurityOps::verifyCRC(uint32_t crcChecksum) 
{
	/*std::stringstream res;
	res << std::hex << std::crc32(packetData.c_str(), packetData.length());
	uint32_t resChecksum;
	res >> resChecksum;*/
	uLong resChecksum = crc32(0L, Z_NULL, 0);
	resChecksum = crc32(resChecksum, reinterpret_cast<const Bytef*>(packetData.c_str()), packetData.length());
	Log().info(__func__, "Our Checksum:", crcChecksum, " Gen Checksum:", static_cast<uint32_t>(resChecksum));
	if (static_cast<uint32_t>(resChecksum) == crcChecksum)
		return true;
	return false;
}

bool SecurityOps::verfiyEncryption()
{

}
