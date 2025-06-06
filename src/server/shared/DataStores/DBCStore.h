/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef DBCSTORE_H
#define DBCSTORE_H

#include "DatabaseEnv.h"
#include "DatabaseWorkerPool.h"
#include "DBCFileLoader.h"
#include "Field.h"
#include "Implementation/WorldDatabase.h"
#include "Log.h"

struct SqlDbc
{
    std::string const* formatString;
    std::string const* indexName;
    std::string sqlTableName;
    int32 indexPos;
    int32 sqlIndexPos;
    SqlDbc(std::string const* _filename, std::string const* _format, std::string const* _idname, char const* fmt)
        : formatString(_format), indexName(_idname), sqlIndexPos(0)
    {
        // Convert dbc file name to sql table name
        sqlTableName = *_filename;
        for (uint32 i = 0; i < sqlTableName.size(); ++i)
        {
            if (isalpha(sqlTableName[i]))
                sqlTableName[i] = char(tolower(sqlTableName[i]));
            else if (sqlTableName[i] == '.')
                sqlTableName[i] = '_';
        }

        // Get sql index position
        DBCFileLoader::GetFormatRecordSize(fmt, &indexPos);
        if (indexPos >= 0)
        {
            uint32 uindexPos = uint32(indexPos);
            for (uint32 x = 0; x < formatString->size(); ++x)
            {
                // Count only fields present in sql
                if ((*formatString)[x] == FT_SQL_PRESENT)
                {
                    if (x == uindexPos)
                        break;
                    ++sqlIndexPos;
                }
            }
        }
    }
private:
    SqlDbc(SqlDbc const& right) = delete;
    SqlDbc& operator=(SqlDbc const& right) = delete;
};

template<class T>
class DBCStorage
{
    typedef std::list<char*> StringPoolList;
public:
    explicit DBCStorage(char const* f)
        : fmt(f), nCount(0), fieldCount(0), dataTable(NULL)
    {
        indexTable.asT = NULL;
    }

    ~DBCStorage() { Clear(); }

    T const* LookupEntry(uint32 id) const
    {
        return (id >= nCount) ? NULL : indexTable.asT[id];
    }

    uint32  GetNumRows() const { return nCount; }
    char const* GetFormat() const { return fmt; }
    uint32 GetFieldCount() const { return fieldCount; }

    bool Load(char const* fn, SqlDbc* sql)
    {
        DBCFileLoader dbc;
        // Check if load was sucessful, only then continue
        if (!dbc.Load(fn, fmt))
            return false;

        uint32 sqlRecordCount = 0;
        uint32 sqlHighestIndex = 0;
        Field* fields = NULL;
        QueryResult result = QueryResult(NULL);
        // Load data from sql
        if (sql)
        {
            std::string query = "SELECT * FROM " + sql->sqlTableName;
            if (sql->indexPos >= 0)
                query += " ORDER BY " + *sql->indexName + " DESC";
            query += ';';


            result = WorldDatabase.Query(query.c_str());
            if (result)
            {
                sqlRecordCount = uint32(result->GetRowCount());
                if (sql->indexPos >= 0)
                {
                    fields = result->Fetch();
                    sqlHighestIndex = fields[sql->sqlIndexPos].GetUInt32();
                }

                // Check if sql index pos is valid
                if (int32(result->GetFieldCount() - 1) < sql->sqlIndexPos)
                {
                    SF_LOG_ERROR("server.loading", "Invalid index pos for dbc:'%s'", sql->sqlTableName.c_str());
                    return false;
                }
            }
        }

        char* sqlDataTable;
        fieldCount = dbc.GetCols();

        dataTable = reinterpret_cast<T*>(dbc.AutoProduceData(fmt, nCount, indexTable.asChar,
            sqlRecordCount, sqlHighestIndex, sqlDataTable));

        stringPoolList.push_back(dbc.AutoProduceStrings(fmt, reinterpret_cast<char*>(dataTable)));

        // Insert sql data into arrays
        if (result)
        {
            if (indexTable.asT)
            {
                uint32 offset = 0;
                uint32 rowIndex = dbc.GetNumRows();
                do
                {
                    if (!fields)
                        fields = result->Fetch();

                    if (sql->indexPos >= 0)
                    {
                        uint32 id = fields[sql->sqlIndexPos].GetUInt32();
                        if (indexTable.asT[id])
                        {
                            SF_LOG_ERROR("server.loading", "Index %d already exists in dbc:'%s'", id, sql->sqlTableName.c_str());
                            return false;
                        }

                        indexTable.asT[id] = reinterpret_cast<T*>(&sqlDataTable[offset]);
                    }
                    else
                        indexTable.asT[rowIndex] = reinterpret_cast<T*>(&sqlDataTable[offset]);

                    uint32 columnNumber = 0;
                    uint32 sqlColumnNumber = 0;

                    for (; columnNumber < sql->formatString->size(); ++columnNumber)
                    {
                        if ((*sql->formatString)[columnNumber] == FT_SQL_ABSENT)
                        {
                            switch (fmt[columnNumber])
                            {
                                case FT_FLOAT:
                                    *reinterpret_cast<float*>(&sqlDataTable[offset]) = 0.0f;
                                    offset += 4;
                                    break;
                                case FT_IND:
                                case FT_INT:
                                    *reinterpret_cast<uint32*>(&sqlDataTable[offset]) = uint32(0);
                                    offset += 4;
                                    break;
                                case FT_BYTE:
                                    *reinterpret_cast<uint8*>(&sqlDataTable[offset]) = uint8(0);
                                    offset += 1;
                                    break;
                                case FT_STRING:
                                    // Beginning of the pool - empty string
                                    *reinterpret_cast<char**>(&sqlDataTable[offset]) = stringPoolList.back();
                                    offset += sizeof(char*);
                                    break;
                            }
                        }
                        else if ((*sql->formatString)[columnNumber] == FT_SQL_PRESENT)
                        {
                            bool validSqlColumn = true;
                            switch (fmt[columnNumber])
                            {
                                case FT_FLOAT:
                                    *reinterpret_cast<float*>(&sqlDataTable[offset]) = fields[sqlColumnNumber].GetFloat();
                                    offset += 4;
                                    break;
                                case FT_IND:
                                case FT_INT:
                                    *reinterpret_cast<uint32*>(&sqlDataTable[offset]) = fields[sqlColumnNumber].GetUInt32();
                                    offset += 4;
                                    break;
                                case FT_BYTE:
                                    *reinterpret_cast<uint8*>(&sqlDataTable[offset]) = fields[sqlColumnNumber].GetUInt8();
                                    offset += 1;
                                    break;
                                case FT_STRING:
                                    SF_LOG_ERROR("server.loading", "Unsupported data type in table '%s' at char %d", sql->sqlTableName.c_str(), columnNumber);
                                    return false;
                                case FT_SORT:
                                    break;
                                default:
                                    validSqlColumn = false;
                                    break;
                            }
                            if (validSqlColumn && (columnNumber != (sql->formatString->size() - 1)))
                                sqlColumnNumber++;
                        }
                        else
                        {
                            SF_LOG_ERROR("server.loading", "Incorrect sql format string '%s' at char %d", sql->sqlTableName.c_str(), columnNumber);
                            return false;
                        }
                    }

                    if (sqlColumnNumber != (result->GetFieldCount() - 1))
                    {
                        SF_LOG_ERROR("server.loading", "SQL and DBC format strings are not matching for table: '%s'", sql->sqlTableName.c_str());
                        return false;
                    }

                    fields = NULL;
                    ++rowIndex;
                } while (result->NextRow());
            }
        }

        // error in dbc file at loading if NULL
        return indexTable.asT != NULL;
    }

    bool LoadStringsFrom(char const* fn)
    {
        // DBC must be already loaded using Load
        if (!indexTable.asT)
            return false;

        DBCFileLoader dbc;
        // Check if load was successful, only then continue
        if (!dbc.Load(fn, fmt))
            return false;

        stringPoolList.push_back(dbc.AutoProduceStrings(fmt, reinterpret_cast<char*>(dataTable)));

        return true;
    }

    void Clear()
    {
        if (!indexTable.asT)
            return;

        delete[] reinterpret_cast<char*>(indexTable.asT);
        indexTable.asT = NULL;
        delete[] reinterpret_cast<char*>(dataTable);
        dataTable = NULL;

        while (!stringPoolList.empty())
        {
            delete[] stringPoolList.front();
            stringPoolList.pop_front();
        }

        nCount = 0;
    }

private:
    char const* fmt;
    uint32 nCount;
    uint32 fieldCount;

    union
    {
        T** asT;
        char** asChar;
    }
    indexTable;

    T* dataTable;
    StringPoolList stringPoolList;
    DBCStorage(DBCStorage const& right) = delete;
    DBCStorage& operator=(DBCStorage const& right) = delete;
};

#endif
