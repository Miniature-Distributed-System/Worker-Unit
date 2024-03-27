#include "test.hpp"
#include "../include/logger.hpp"

int Tester::exactMatch(std::string function, std::string actualResult, 
				std::string expectedResult, std::string identifier)
{
	if(actualResult == expectedResult)
		Log().success(function, "Actual result:", actualResult, " Expected Result:", expectedResult, " success");
	else
		Log().error(function, "Actual result:", actualResult, " Expected Result:", expectedResult, " failed");
}

int Tester::keywordMatch(std::string function, std::string keyword, 
				std::string matchString)
{
	if (matchString.find(keyword) != std::string::npos) {
        Log().success(function, "Keyword ", keyword, " present in ", matchString);
    } else {
        Log().error(function, "Keyword ", keyword, " not present in ", matchString);
    }
}

void checkKeysRecursive(const nlohmann::json& jsonObject, const std::vector<std::string>& keysToCheck, 
				std::vector<std::string>& missingKeys, std::string currentKey = "") {
    if (jsonObject.is_object()) {
        for (const auto& [key, value] : jsonObject.items()) {
            std::string newKey = currentKey.empty() ? key : currentKey + "." + key;
            if (std::find(keysToCheck.begin(), keysToCheck.end(), newKey) != keysToCheck.end()) {
                // Found a key to check
                if (value.is_null()) {
                    missingKeys.push_back(newKey);
                }
            }
            if (value.is_object() || value.is_array()) {
                // Recursive call for nested object or array
                checkKeysRecursive(value, keysToCheck, missingKeys, newKey);
            }
        }
    }
}
int jsonKeyMatch(std::string function, std::string actualJsonString, 
						std::vector<std::string> matchingStrings, std::string identifier)
{
	try{
		nlohmann::json jsonObject = nlohmann::json::parse(actualJsonString);
		std::vector<std::string> missingKeys;
		checkKeysRecursive(jsonObject, matchingStrings, missingKeys);
		if(missingKeys.size() > 0){
			for(std::string err : missingKeys){
				Log().error(function, "Key not present:", missingKeys);
			Log().error(function, "Total missing keys:", missingKeys.size());
			return 0;
			}
		}
		Log().success(function, "No missing keys");
	} catch(std::exception const& e) {
        Log().error(__func__, "Error: ", e.what());
    }
}

void compareJSON(const nlohmann::json& json1, const nlohmann::json& json2, 
				std::vector<std::string>& mismatchedKeys) {
    if (json1.is_object() && json2.is_object()) {
        // Check if json1 has keys missing in json2 or values mismatch
        for (auto it = json1.begin(); it != json1.end(); ++it) {
            auto key = it.key();
            if (!json2.contains(key)) {
                mismatchedKeys.push_back(key);
            } else {
                compareJSON(json1[key], json2[key], mismatchedKeys);
            }
        }
    } else {
        // If not both JSON objects, compare values directly
        if (json1 != json2) {
            // If values are different, add key to mismatchedKeys
            mismatchedKeys.push_back(json1.dump());
        }
    }
}

int Tester::jsonMatch(std::string function, std::string actualJsonString, 
				std::string expectedJsonString, std::string identifier)
{
	try{
		nlohmann::json jsonObject = nlohmann::json::parse(actualJsonString);
		nlohmann::json expectedJsonObject = nlohmann::json::parse(expectedJsonString);
		std::vector<std::string> mismatchKeys;
		
		compareJSON(expectedJsonObject, jsonObject, mismatchKeys);
		if(mismatchKeys.size() > 0){
			for(std::string error : mismatchKeys)
				Log().error(function, "missmatch key:value ", error);
			Log().error(function, "Total missmatches reported:", mismatchKeys.size());
		}
		Log().success(function, "no json missmatch");
	} catch(std::exception const& e) {
        Log().error(__func__, "Error: ", e.what());
    }
}
