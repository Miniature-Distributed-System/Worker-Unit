#include <sstream>
#include <regex>
#include <filesystem>
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "file_database_access.hpp"

// class FileDataBaseExceptions : public std::exception {
//         std::string message;
//     public:
//         FileDataBaseExceptions(std::string str) : message(str) {};
//         const char * what() const override { return message.c_str(); };
// };

FileDataBaseAccess::FileDataBaseAccess(std::string fileName, FileAccessType accessMacro) : 
                fileName(fileName) ,accessMode(accessMacro)
{
    this->fileName = fileName + ".csv";
    try{
        if(accessMode == READ_FILE){
            fileAccess.open(fileName, std::fstream::in);
            std::string buffer;
            while(getline(fileAccess, buffer)){
                readData.push_back(buffer);
            }
            Log().info(__func__, "opened file in read mode:", fileName);
            fileAccess.close();
        } else {
            fileAccess.open(fileName, std::fstream::out | std::fstream::in);
            if(fileAccess.fail()){
                Log().info(__func__, fileName, "file does not exist creating new file");
                fileAccess.open(fileName, std::fstream::out);
                fileAccess.close();
                fileAccess.open(fileName, std::fstream::out | std::fstream::in);
            }
            std::string buffer;
            while(getline(fileAccess, buffer)){
                readWriteData.push_back(buffer);
            }
            fileAccess.close();
            Log().info(__func__, "opened file in read/write mode:", fileName);
        }
        dataModified.initFlag(false);
        fileDeleted.initFlag(false);
        
    } catch (std::exception &e){
        Log().error(__func__,"Error in opening file", e.what());
    }   
}

FileDataBaseAccess::~FileDataBaseAccess()
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
        return;
    }
    
    commitChanges();
    fileAccess.open(fileName + ".csv", std::fstream::in);
    //DEBUG_MSG(__func__, "commited changes are:", fileAccess.rdbuf());
    fileAccess.close();
    Log().info(__func__, "closing file:", fileName, " with access mode:", accessMode);
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


/* writeBlob(): This method is used to write passed data blob argument to flat file.
* This method writes a single row of data to the flat file. The string data format shoud be of type CSV.
* The write operation is only allowed if the file is opened in RW mode.
*/
int FileDataBaseAccess::writeBlob(std::string data)
{
    if(fileDeleted.isFlagSet()){
        Log().info(__func__, "File was deleted and won't save any changes");
        return ACCESS_DENIED;
    }

    if(accessMode == READ_FILE)
        return INVALID_ACCESS_MODE;

    readWriteData = split_string_by_newline(data);
    dataModified.setFlag();

    return 0;
}

/* getBlob(): This method returns the entire formated flat file contents in from of string.
*/
std::string FileDataBaseAccess::getBlob()
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
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

/* getTotalRows(): Returns the total number of rows of the whole formatted file
*/
int FileDataBaseAccess::getTotalRows()
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
        return -1;
    }

    if(accessMode == READ_FILE){
        return readData.size();
    }
    
    return readWriteData.size();
}

/* getTotalColumns(): Returns the total number of columns of the whole formatted file
*/
int FileDataBaseAccess::getTotalColumns()
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
        return ACCESS_DENIED;
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

/* getRowValueList(): This method returns all the values of a particular row.
* The method takes a row index and as arguement and returns the row values as a list of string values. It takes care of
* delimeters and returns the values ommiting the delimitters. The method also accounts for overflow index and returns
* empty list if index is overflowing.
*/
std::vector<std::string> FileDataBaseAccess::getRowValueList(int rowIndex)
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
        std::vector<std::string> empty;
        return empty;
    }

    if(rowIndex < 0){
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

/* getRowValueList(): This method returns all the values of the Column header Values/Names.
* The method returns the column values/names as a list of string values. It takes care of delimeters and returns the
* values ommiting the delimitters.
*/
std::vector<std::string> FileDataBaseAccess::getColumnNamesList()
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
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

/* writeRowValue(): Method writes the passed string value into the row and column specified.
* This method accounts for both row and column indexs and returns error numbers based on the overflow type. This method
* works only if file opened in RW_FILE mode. It automatically adds delimiters and values should not have ',' as character
*/
int FileDataBaseAccess::writeRowValue(std::string value, int rowIndex, int columnIndex)
{
    if(fileDeleted.isFlagSet()){
         Log().error(__func__, "File is deleted and can't be accessed");
         return ACCESS_DENIED;
    }

    if(accessMode == READ_FILE){
        Log().error(__func__, "Writing is not allowed in read mode");
        return INVALID_ACCESS_MODE;
    }
    if(rowIndex > readWriteData.size()){
        Log().debug(__func__, "row index has overflowed");
        return ROW_INDEX_OVERFLOW;
    }

    if(columnIndex > getTotalColumns()){
        Log().debug(__func__, "column index has overflowed");
        return COLUMN_INDEX_OVERFLOW;
    }

    if(rowIndex < 0)
        return ROW_INDEX_UNDERFLOW;
    if(columnIndex < 0)
        return COLUMN_INDEX_UNDERFLOW;

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

/* writeRowValueList(): Method writes the passed list of string values into the row and column specified.
* This method accounts for both row and column indexs and returns error numbers based on the overflow type. This method
* works only if file opened in RW_FILE mode. It automatically adds delimiters and values should not have ',' as character
*/
int FileDataBaseAccess::writeRowValueList(std::vector<std::string> valueList, int rowIndex)
{
    if(fileDeleted.isFlagSet()){
         Log().error(__func__, "File is deleted and can't be accessed");
         return ACCESS_DENIED;
    }

    if(accessMode == READ_FILE){
        Log().error(__func__, "Writing is not allowed in read mode");
        return INVALID_ACCESS_MODE;
    }
    if(rowIndex > readWriteData.size()){
        Log().debug(__func__, "row index has overflowed");
        return ROW_INDEX_OVERFLOW;
    }

    if(rowIndex < 0)
        return ROW_INDEX_UNDERFLOW;

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

/* getRowValue(): This method returns a single value at the passed row and column index.
* This method accouynts for row and column index overflow and returns empty string.
*/
std::string FileDataBaseAccess::getRowValue(int rowIndex, int columnIndex)
{
    if(fileDeleted.isFlagSet()){
         Log().error(__func__, "File is deleted and can't be accessed");
         return "";
    }

    if(rowIndex > readWriteData.size()){
        Log().debug(__func__, "row index has overflowed");
        return "";
    }

    if(columnIndex > getTotalColumns()){
        Log().debug(__func__, "column index has overflowed");
        return "";
    }

    if(rowIndex < 0)
        return "";
    if(columnIndex < 0)
        return "";

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

/* deleteDuplicateRecords(): Specialized method for dataprocessor only which deletes duplicate records in file. 
*/
int FileDataBaseAccess::deleteDuplicateRecords(int rowIndex)
{
    if(fileDeleted.isFlagSet()){
         Log().error(__func__, "File is deleted and can't be accessed");
         return ACCESS_DENIED;
    }

    if(accessMode == READ_FILE){
        Log().error(__func__, "Writing is not allowed in read mode");
        return INVALID_ACCESS_MODE;
    }
    if(readWriteData.size() <= rowIndex){
        Log().info(__func__, "Deuplicate rows deletion has come to end of record");
        return DUPLICATE_CLEANUP_DONE;
    }

    if(rowIndex < 0)
        return ROW_INDEX_UNDERFLOW;
        
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

/* dropFile(): This method deletes file from system.
* Can be executed only if file opened in RW_FILE mode. This also automatically closes the file.
*/
void FileDataBaseAccess::dropFile()
{
    if(fileDeleted.isFlagSet()){
         Log().error(__func__, "File is deleted and can't be accessed");
         return;
    }

    if(accessMode == READ_FILE){
        Log().error(__func__, "Writing is not allowed in read mode");
        return;
    }
    
    fileAccess.close();
    std::filesystem::remove(fileName);
    fileDeleted.setFlag();
}

/* commitChanges(): Used to push saved changes to file.
* This method psuhes all the saved formatted string data into the opened file. Works only in RW_FILE mode. This does
* not close file and file can be used for write opertaion using same object. 
*/
void FileDataBaseAccess::commitChanges()
{
    // Commit changes done after write and if modifed
    if(accessMode == RW_FILE && dataModified.isFlagSet()){
        std::ofstream writeAccess(fileName, std::fstream::out);
    }
}