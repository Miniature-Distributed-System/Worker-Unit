#include <sstream>
#include <regex>
#include <filesystem>
#include "../include/debug_rp.hpp"
#include "database_access.hpp"

class RowOutOfBounds : public std::exception {
    using std::exception::exception;
};

FileDataBaseAccess::FileDataBaseAccess(std::string fileName, FileAccessType accessMacro) : 
                fileName(fileName) ,accessMode(accessMacro)
{
    try{
        if(accessMode == READ_FILE){
            fileAccess.open(fileName + ".csv", std::fstream::in);
            std::string buffer;
            while(getline(fileAccess, buffer)){
                readData.push_back(buffer);
            }
            DEBUG_MSG(__func__, "opened file in read mode:", fileName);
            fileAccess.close();
        } else {
            fileAccess.open(fileName + ".csv", std::fstream::out | std::fstream::in);
            if(fileAccess.fail()){
                DEBUG_MSG(__func__, fileName, ".csv file does not exist creating new file");
                fileAccess.open(fileName + ".csv", std::fstream::out);
                fileAccess.close();
                fileAccess.open(fileName + ".csv", std::fstream::out | std::fstream::in);
            }
            std::string buffer;
            while(getline(fileAccess, buffer)){
                readWriteData.push_back(buffer);
            }
            fileAccess.close();
            DEBUG_MSG(__func__, "opened file in read/write mode:", fileName);
        }
        dataModified.initFlag(false);
        fileDeleted.initFlag(false);
        
    } catch (std::exception &e){
        DEBUG_ERR(__func__,"Error in opening file", e.what());
    }   
}

FileDataBaseAccess::~FileDataBaseAccess()
{
    if(fileDeleted.isFlagSet()){
        DEBUG_ERR(__func__, "File is deleted and can't be accessed");
        return;
    }
    
    commitChanges();
    fileAccess.open(fileName + ".csv", std::fstream::in);
    DEBUG_MSG(__func__, "commited changes are:", fileAccess.rdbuf());
    fileAccess.close();
    DEBUG_MSG(__func__, "closing file:", fileName, " with access mode:", accessMode);
}

// Pulled off from stackoverflow
std::list<std::string> split_string_by_newline(const std::string& str)
{
    auto result = std::list<std::string>{};
    auto ss = std::stringstream{str};

    for (std::string line; std::getline(ss, line, '\n');)
        result.push_back(line);

    return result;
}

int FileDataBaseAccess::writeBlob(std::string data)
{
    if(fileDeleted.isFlagSet()){
        DEBUG_MSG(__func__, "File was deleted and won't save any changes");
        return -1;
    }

    if(accessMode == READ_FILE)
        throw "Write not permitted in read mode";
    readWriteData = split_string_by_newline(data);
    dataModified.setFlag();

    return 0;
}

std::string FileDataBaseAccess::getBlob()
{
    if(fileDeleted.isFlagSet()){
        DEBUG_ERR(__func__, "File is deleted and can't be accessed");
        return "";
    }

    if(accessMode == READ_FILE){
        std::string resultData;
        for(auto& row : readData)
            resultData += row + '\n';
        return resultData;
    } else {
        std::string resultData;
        for(auto i = readWriteData.begin(); i != readWriteData.end(); i++)
            resultData += (*i) + "\n";
        return resultData;
    }
}

int FileDataBaseAccess::getTotalRows()
{
    if(fileDeleted.isFlagSet()){
        DEBUG_ERR(__func__, "File is deleted and can't be accessed");
        return -1;
    }

    if(accessMode == READ_FILE){
        return readData.size();
    }
    
    return readWriteData.size();
}

int FileDataBaseAccess::getTotalColumns()
{
    if(fileDeleted.isFlagSet()){
        DEBUG_ERR(__func__, "File is deleted and can't be accessed");
        return -1;
    }
        
    std::string data;
    int totalColumns = 0;

    if(accessMode == READ_FILE){
        data = readData.front();
    } else {
        data = readWriteData.front();
    }
    
    for(int i = 0, total = 0; i < data.length(); i++)
        if(data[i] == ',')
            totalColumns++;

    // There is one less ',' than that of total columns
    return totalColumns + 1;
}

// Pulled off from geeksforgeeks
std::vector<std::string> adv_tokenizer(std::string s, char del)
{
    std::stringstream ss(s);
    std::string word;
    std::vector<std::string> result;
    while (!ss.eof()) {
        std::getline(ss, word, del);
        result.push_back(word);
    }

    return result;
}

std::vector<std::string> FileDataBaseAccess::getRowValueList(int rowIndex)
{
    if(fileDeleted.isFlagSet()){
        DEBUG_ERR(__func__, "File is deleted and can't be accessed");
        std::vector<std::string> empty;
        return empty;
    }

    std::list<std::string> resultData;
    std::string data;

    if(accessMode == READ_FILE){
        data = readData[rowIndex];
    } else {
        int k = 0;
        for(auto i = readWriteData.begin(); i != readWriteData.end(); i++, k++){
            if(k == rowIndex){
                data = *i;
                break;
            }
        }
    }

    return adv_tokenizer(data, ',');
}

std::vector<std::string> FileDataBaseAccess::getColumnNamesList()
{
    if(fileDeleted.isFlagSet()){
        DEBUG_ERR(__func__, "File is deleted and can't be accessed");
        std::vector<std::string> empty;
        return empty;
    }

    std::string data;

    if(accessMode == READ_FILE){
        data = readData[0];
    } else {
        data = readWriteData.front();
    }

    return adv_tokenizer(data, ',');
}

int FileDataBaseAccess::writeRowValue(std::string value, int rowIndex, int columnIndex)
{
    if(fileDeleted.isFlagSet()){
         DEBUG_ERR(__func__, "File is deleted and can't be accessed");
         return -1;
    }

    if(accessMode == READ_FILE){
        DEBUG_ERR(__func__, "Writing is not allowed in read mode");
        return -2;
    }
    if(readWriteData.size() < rowIndex){
        DEBUG_ERR(__func__, "row index exceeds total rows present in CSV");
        return -3;
    }

    std::string data;
    std::string replacementString;
    int k = 0;

    auto i = readWriteData.begin();
    for(; i != readWriteData.end(); i++, k++){
        if(k == rowIndex){
            data = *i;
            break;
        }
    }

    data = std::regex_replace(data, std::regex(adv_tokenizer(data, ',')[columnIndex]), value);
    // i moves forward and insert inserts before i th element
    readWriteData.erase(i);
    readWriteData.insert(i, data);
    dataModified.setFlag();

    return 0;
}

int FileDataBaseAccess::writeRowValueList(std::vector<std::string> valueList, int rowIndex)
{
    if(fileDeleted.isFlagSet()){
         DEBUG_ERR(__func__, "File is deleted and can't be accessed");
         return -1;
    }

    if(accessMode == READ_FILE){
        DEBUG_ERR(__func__, "Writing is not allowed in read mode");
        return -2;
    }
    if(readWriteData.size() < rowIndex){
        DEBUG_ERR(__func__, "row index exceeds total rows present in CSV");
        return -3;
    }

    std::string rowString = valueList[0];
    for(int i = 1; i < valueList.size(); i++)
        rowString += "," + valueList[i];
    auto i = readWriteData.begin();
    // Move iterator to the desired row index
    for(int j = 0; j != rowIndex; j++) i++;
    i = readWriteData.erase(i);
    readWriteData.insert(i, rowString);

    return 0;
}

std::string FileDataBaseAccess::getRowValue(int rowIndex, int columnIndex)
{
    if(fileDeleted.isFlagSet()){
         DEBUG_ERR(__func__, "File is deleted and can't be accessed");
         return "";
    }

    std::string data;
    int k;

    if(accessMode == READ_FILE)
        data = readData[rowIndex];
    else {
        k = 0;
        for(auto i = readWriteData.begin(); i != readWriteData.end(); i++, k++){
            if(k == rowIndex){
                data = *i;
                break;
            }
        }
    }
    return adv_tokenizer(data, ',')[columnIndex]; 
}

int FileDataBaseAccess::deleteDuplicateRecords(int rowIndex)
{
    if(fileDeleted.isFlagSet()){
         DEBUG_ERR(__func__, "File is deleted and can't be accessed");
         return -1;
    }

    if(accessMode == READ_FILE){
        DEBUG_ERR(__func__, "Writing is not allowed in read mode");
        return -2;
    }
    if(readWriteData.size() <= rowIndex){
        DEBUG_MSG(__func__, "Deuplicate rows deletion has come to end of record");
        return -3;
    }
        
    auto iterator = readWriteData.begin();
    int k = 0;
    while(k++ != rowIndex){
        iterator++;
    }
    k = 0;
    std::string compareString = *iterator;
    for(auto iteratorString = ++iterator; iteratorString != readWriteData.end(); iteratorString++, k++){
        if(!(compareString).compare(*iteratorString)){
            readWriteData.erase(iteratorString--);
            dataModified.setFlag();
        }
    }

    return 0;
}

void FileDataBaseAccess::dropFile()
{
    if(fileDeleted.isFlagSet()){
         DEBUG_ERR(__func__, "File is deleted and can't be accessed");
         return;
    }

    if(accessMode == READ_FILE){
        DEBUG_ERR(__func__, "Writing is not allowed in read mode");
        return;
    }
    
    fileAccess.close();
    std::filesystem::remove(fileName + ".csv");
    fileDeleted.setFlag();
}

void FileDataBaseAccess::commitChanges()
{
    // Commit changes done after write and if modifed
    if(accessMode == RW_FILE && dataModified.isFlagSet()){
        std::ofstream writeAccess(fileName + ".csv", std::fstream::out);
        std::string buffer;
        for(auto i = readWriteData.begin(); i != readWriteData.end(); i++){
            buffer += (*i) + '\n';
        }
        writeAccess << buffer;
        writeAccess.close();
    }
}