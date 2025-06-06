/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef DB2_FILE_LOADER_H
#define DB2_FILE_LOADER_H

#include "Define.h"
#include "Utilities/ByteConverter.h"
#include <cassert>
#include <string>

class DB2FileLoader
{
public:
    DB2FileLoader() : recordSize(0), recordCount(0), fieldCount(0), stringSize(0), data(NULL), fieldsOffset(NULL), stringTable(NULL),
        tableHash(0), build(0), unk1(0), minIndex(0), maxIndex(0), locale(0), unk5(0) { }

    ~DB2FileLoader();

    bool Load(const char* filename, const char* fmt);

    class Record
    {
    public:
        float getFloat(size_t field) const
        {
            assert(field < file.fieldCount);
            float val = *reinterpret_cast<float*>(offset + file.GetOffset(field));
            EndianConvert(val);
            return val;
        }
        uint32 getUInt(size_t field) const
        {
            assert(field < file.fieldCount);
            uint32 val = *reinterpret_cast<uint32*>(offset + file.GetOffset(field));
            EndianConvert(val);
            return val;
        }
        uint8 getUInt8(size_t field) const
        {
            assert(field < file.fieldCount);
            return *reinterpret_cast<uint8*>(offset + file.GetOffset(field));
        }

        const char* getString(size_t field) const
        {
            assert(field < file.fieldCount);
            size_t stringOffset = getUInt(field);
            assert(stringOffset < file.stringSize);
            return reinterpret_cast<char*>(file.stringTable + stringOffset);
        }

    private:
        Record(DB2FileLoader& file_, unsigned char* offset_) : offset(offset_), file(file_) {}
        unsigned char* offset;
        DB2FileLoader& file;

        friend class DB2FileLoader;
    };

    // Get record by id
    Record getRecord(size_t id);
    /// Get begin iterator over records

    uint32 GetNumRows() const { return recordCount; }
    uint32 GetCols() const { return fieldCount; }
    uint32 GetOffset(size_t id) const { return (fieldsOffset != NULL && id < fieldCount) ? fieldsOffset[id] : 0; }
    uint32 GetHash() const { return tableHash; }
    bool IsLoaded() const { return (data != NULL); }
    char* AutoProduceData(std::string fmt, uint32& count, char**& indexTable);
    char* AutoProduceStringsArrayHolders(std::string fmt, char* dataTable);
    char* AutoProduceStrings(std::string fmt, char* dataTable, uint32 locale);
    static uint32 GetFormatRecordSize(std::string format, int32* index_pos = NULL);
    static uint32 GetFormatStringsFields(std::string format);

private:
    uint32 recordSize;
    uint32 recordCount;
    uint32 fieldCount;
    uint32 stringSize;
    uint32* fieldsOffset;
    unsigned char* data;
    unsigned char* stringTable;

    // WDB2 / WCH2 fields
    uint32 tableHash;    // WDB2
    uint32 build;        // WDB2

    int unk1;            // WDB2 (Unix time in WCH2)
    int minIndex;        // WDB2
    int maxIndex;        // WDB2 (index table)
    int locale;          // WDB2
    int unk5;            // WDB2
};

#endif