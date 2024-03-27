#ifndef TESTER_HPP
#define TESTER_HPP

#include <vector>
#include "../lib/nlohmann/json-schema.hpp"

class Tester {
	public:
		int exactMatch(std::string function, std::string actualResult, 
						std::string expectedResult, std::string identifier);
		int keywordMatch(std::string function, std::string keyword, 
						std::string matchString);
		int jsonMatch(std::string function, std::string actualJsonString, 
						std::string expectedJsonString, std::string identifier);

		int jsonKeyMatch(std::string function, std::string actualJsonString, 
						std::vector<std::string> matchingStrings, std::string identifier);
};
#endif
