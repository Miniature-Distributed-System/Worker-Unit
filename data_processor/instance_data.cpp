#include "../services/sqlite_database_access.hpp"
#include "../include/debug_rp.hpp"
#include "../include/logger.hpp"
#include "instance_data.hpp"

InstanceData::InstanceData(Instance instance, int cols, std::string userTableId) : instance(instance), 
                userTableName(userTableId) , userTableTotalColumns(cols)
{
    fileDataBaseAccess = new FileDataBaseAccess(userTableName, READ_FILE);
}

std::string* InstanceData::validateColumns()
{
    int k = 0, cols = instance.getTotalColumns();

    if(cols != userTableTotalColumns){
        errorString = "instance column count doesn't match with userdata columns,  ";
        errorString += " instance columns: " + std::to_string(cols) + " userdata columns:" + 
                        std::to_string(userTableTotalColumns);
        Log().debug(__func__, errorString);
        return NULL;
    }

    std::string *result = new std::string[cols];
    std::string *instanceColumnNames = sqliteDatabaseAccess->getColumnNames(instance.getId(), cols);
    std::vector<std::string> userTableColumnNames = fileDataBaseAccess->getColumnNamesList();

    if(instanceColumnNames == NULL || userTableColumnNames.size() == 0){
        Log().error(__func__,"column data retrevial failed");
        return NULL;
    }

    //Match all columns else its an error
    for(int i = 0; i < cols; i++){
        for(int j = 0; j < cols; j++){
            if(!userTableColumnNames[i].compare(instanceColumnNames[j])){
                result[k++] = instanceColumnNames[j];
                break;
            }
        }
    }

    if(k != cols){
        errorString = "Instance table and User table columns did not match";
        Log().debug(__func__, errorString);
        return NULL;
    }

    Log().dataProcInfo(__func__, "Columns validation successfull!");
    return result;
}

int InstanceData::initlizeData()
{
    std::string *str, *columnNames;

    columnNames = validateColumns();
    if(columnNames == NULL)
        return -1;
    for(int i = 0; i < instance.getTotalColumns(); i++){
        //Log().dataProcInfo(__func__, "TableId:", instance.getId(), " ColName:", columnNames[i]);
        //Any failures here would be unfortunate and not correctable
        str = sqliteDatabaseAccess->getColumnValues(instance.getId(), columnNames[i], instance.getTotalRows());
        instanceDataMatrix.push_back(str);
    }

    Log().dataProcInfo(__func__, "data initilize success!");
    return 0;
}

std::string* InstanceData::getPossibleFields(int column)
{
    return instanceDataMatrix.at(column);
}