#include <sstream>
#include <regex>
#include <filesystem>
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "file_database_access.hpp"

FileDataBaseAccess::FileDataBaseAccess(std::string fileName, FileAccessType accessMacro) : 
                fileName(fileName) ,accessMode(accessMacro)
{
    this->fileName = fileName + ".csv";
    try{
        if(accessMode == READ_FILE){
            fileAccess.open(this->fileName, std::fstream::in);
            std::string buffer;
            while(getline(fileAccess, buffer)){
                readData.push_back(buffer);
            }
            Log().info(__func__, "opened file in read mode:", this->fileName);
            fileAccess.close();
        } else {
            fileAccess.open(this->fileName, std::fstream::out | std::fstream::in);
            if(fileAccess.fail()){
                Log().info(__func__, this->fileName, "file does not exist creating new file");
                fileAccess.open(this->fileName, std::fstream::out);
                fileAccess.close();
                fileAccess.open(this->fileName, std::fstream::out | std::fstream::in);
            }
            std::string buffer;
            while(getline(fileAccess, buffer)){
                readWriteData.push_back(buffer);
            }
            fileAccess.close();
            Log().info(__func__, "opened file in read/write mode:", this->fileName);
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
    fileAccess.open(fileName, std::fstream::in);
    fileAccess.close();
    Log().info(__func__, "closing file:", fileName, " with access mode:", accessMode);
}

IDataContainer& FileDataBaseAccess::getContainer()
{
    if (accessMode == RW_FILE)
        return readWriteData;
    return readData;
}

std::string VectorContainer::getRow(int index)
{
	return dataContainer[index];
}

std::string VectorContainer::getBlob()
{
	std::string buffer;
    for(const auto &line : dataContainer){
        buffer += line + "\n";
    }
	return buffer;
}

void VectorContainer::deleteRow(int index)
{
    dataContainer.erase(dataContainer.begin() + index);
}

void VectorContainer::replaceRow(int index, std::string rowValue)
{
	dataContainer[index] = rowValue;
}

std::size_t VectorContainer::size() const
{
    return dataContainer.size();
}

std::string& VectorContainer::front() 
{
    return dataContainer.front();
}

void VectorContainer::push_back(const std::string& value) 
{
    dataContainer.push_back(value);
}

std::string ListContainer::getRow(int index)
{
	int tempIndex = 0;
	for(auto i = dataContainer.begin(); i != dataContainer.end(); i++, tempIndex++)
	{
		if(index == tempIndex) return *i;
	}
	return "";
}

std::size_t ListContainer::size() const 
{
    return dataContainer.size();
}

std::string& ListContainer::front() 
{
	return dataContainer.front();
}

void ListContainer::push_back(const std::string& value) 
{
  	dataContainer.push_back(value);
}

std::string ListContainer::getBlob()
{
	std::string buffer;
	for(const auto &line : dataContainer){
		buffer += line + "\n";
	}
	return buffer;
}

void ListContainer::replaceRow(int index, std::string rowString)
{
	auto i = dataContainer.begin();
    // Move iterator to the desired row index
    for(int j = 0; j != index; j++) i++;
    i = dataContainer.erase(i);
    dataContainer.insert(i, rowString);
}

void ListContainer::deleteRow(int index)
{
	auto it = dataContainer.begin();
	std::advance(it, index);
    dataContainer.erase(it);
}

// Pulled off from stackoverflow
ListContainer split_string_by_newline(const std::string& str)
{
    auto result = ListContainer();
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

	return getContainer().getBlob();
}

/* getTotalRows(): Returns the total number of rows of the whole formatted file
*/
int FileDataBaseAccess::getTotalRows()
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
        return -1;
    }

	return getContainer().size();
}

/* getTotalColumns(): Returns the total number of columns of the whole formatted file
*/
int FileDataBaseAccess::getTotalColumns()
{
    if(fileDeleted.isFlagSet()){
        Log().error(__func__, "File is deleted and can't be accessed");
        return ACCESS_DENIED;
    }
        
    int totalColumns = 0;
	std::string data = getContainer().front();
    
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

    return adv_tokenizer(getContainer().getRow(rowIndex), ',');
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

    return adv_tokenizer(getContainer().front(), ',');
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

	std::string data = readWriteData.getRow(rowIndex);
    data = std::regex_replace(data, std::regex(adv_tokenizer(data, ',')[columnIndex]), value);
	readWriteData.replaceRow(rowIndex, data);
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
	readWriteData.replaceRow(rowIndex, rowString);

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

	std::string data = getContainer().getRow(rowIndex);
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
        Log().info(__func__, "Duplicate rows deletion has come to end of record");
        return DUPLICATE_CLEANUP_DONE;
    }

    if(rowIndex < 0)
        return ROW_INDEX_UNDERFLOW;
        
	std::string compareString = readWriteData.getRow(rowIndex);
	for(int i = rowIndex + 1; i < readWriteData.size(); i++)
	{
		std::string listString = readWriteData.getRow(i);
		if(!(compareString).compare(listString)){
			readWriteData.deleteRow(i);
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
* This method pushes all the saved formatted string data into the opened file. Works only in RW_FILE mode. This does
* not close file and file can be used for write operation using same object. 
*/
void FileDataBaseAccess::commitChanges()
{
    // Commit changes done after write and if modifed
    if(accessMode == RW_FILE && dataModified.isFlagSet()){
		try {
			fileAccess.open(this->fileName, std::fstream::out);
        	if(!fileAccess.fail())
        	{
            	fileAccess << readWriteData.getBlob();
            	fileAccess.close();
        	}
		} catch (const std::ios_base::failure& e) {
			Log().error(__func__, "file db error: ", e.what());
		} catch (const std::exception &e) {
			Log().error(__func__, "Exception: ", e.what());
		}
    }
}
