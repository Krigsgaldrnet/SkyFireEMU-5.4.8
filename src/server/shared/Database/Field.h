/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef _FIELD_H
#define _FIELD_H

#include "Common.h"
#include "Log.h"

#include <mysql.h>
#include <array>

class Field
{
    friend class ResultSet;
    friend class PreparedResultSet;

public:
    [[nodiscard]] bool GetBool() const // Wrapper, actually gets integer
    {
        return (GetUInt8() == 1);
    }

    [[nodiscard]] uint8 GetUInt8() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_TINY))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetUInt8() on non-tinyint field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<uint8*>(data.value);
        return static_cast<uint8>(atol((char*)data.value));
    }

    [[nodiscard]] int8 GetInt8() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_TINY))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetInt8() on non-tinyint field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<int8*>(data.value);
        return static_cast<int8>(atol((char*)data.value));
    }

#ifdef ELUNA
    enum_field_types GetType() const
    {
        return data.type;
    }
#endif

    [[nodiscard]] uint16 GetUInt16() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_SHORT) && !IsType(MYSQL_TYPE_YEAR))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetUInt16() on non-smallint field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<uint16*>(data.value);
        return static_cast<uint16>(atol((char*)data.value));
    }

    [[nodiscard]] int16 GetInt16() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_SHORT) && !IsType(MYSQL_TYPE_YEAR))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetInt16() on non-smallint field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<int16*>(data.value);
        return static_cast<int16>(atol((char*)data.value));
    }

    [[nodiscard]] uint32 GetUInt32() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_INT24) && !IsType(MYSQL_TYPE_LONG))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetUInt32() on non-(medium)int field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<uint32*>(data.value);
        return static_cast<uint32>(atol((char*)data.value));
    }

    [[nodiscard]] int32 GetInt32() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_INT24) && !IsType(MYSQL_TYPE_LONG))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetInt32() on non-(medium)int field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<int32*>(data.value);
        return static_cast<int32>(atol((char*)data.value));
    }

    [[nodiscard]] uint64 GetUInt64() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_LONGLONG) && !IsType(MYSQL_TYPE_BIT))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetUInt64() on non-bigint field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<uint64*>(data.value);
        return static_cast<uint64>(atol((char*)data.value));
    }

    [[nodiscard]] int64 GetInt64() const
    {
        if (!data.value)
            return 0;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_LONGLONG) && !IsType(MYSQL_TYPE_BIT))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetInt64() on non-bigint field. Using type: %s.", FieldTypeToString(data.type));
            return 0;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<int64*>(data.value);
        return static_cast<int64>(strtol((char*)data.value, nullptr, 10));
    }

    [[nodiscard]] float GetFloat() const
    {
        if (!data.value)
            return 0.0f;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_FLOAT))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetFloat() on non-float field. Using type: %s.", FieldTypeToString(data.type));
            return 0.0f;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<float*>(data.value);
        return static_cast<float>(atof((char*)data.value));
    }

    [[nodiscard]] double GetDouble() const
    {
        if (!data.value)
            return 0.0f;

#ifdef SKYFIRE_DEBUG
        if (!IsType(MYSQL_TYPE_DOUBLE))
        {
            SF_LOG_WARN("sql.sql", "Warning: GetDouble() on non-double field. Using type: %s.", FieldTypeToString(data.type));
            return 0.0f;
        }
#endif

        if (data.raw)
            return *reinterpret_cast<double*>(data.value);
        return static_cast<double>(atof((char*)data.value));
    }

    [[nodiscard]] char const* GetCString() const
    {
        if (!data.value)
            return nullptr;

#ifdef SKYFIRE_DEBUG
        if (IsNumeric())
        {
            SF_LOG_WARN("sql.sql", "Error: GetCString() on numeric field. Using type: %s.", FieldTypeToString(data.type));
            return nullptr;
        }
#endif
        return static_cast<char const*>(data.value);
    }

    [[nodiscard]] std::string GetString() const
    {
        if (!data.value)
            return "";

        if (data.raw)
        {
            char const* string = GetCString();
            if (!string)
                string = "";
            return std::string(string, data.length);
        }
        return std::string((char*)data.value);
    }

    [[nodiscard]] bool IsNull() const
    {
        if (IsBinary() && data.length == 0)
        {
            return true;
        }
        return data.value == nullptr;
    }

    [[nodiscard]] std::vector<uint8> GetBinary() const;
    template<size_t S>
    [[nodiscard]] std::array<uint8, S> GetBinary() const
    {
        std::array<uint8, S> buf;
        GetBinarySizeChecked(buf.data(), S);
        return buf;
    }

protected:
    Field();
    ~Field();

#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif
    struct
    {
        uint32 length;          // Length (prepared strings only)
        void* value;            // Actual data in memory
        enum_field_types type;  // Field type
        bool raw;               // Raw bytes? (Prepared statement or ad hoc)
    } data;
#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

    void SetByteValue(void const* newValue, size_t const newSize, enum_field_types newType, uint32 length);
    void SetStructuredValue(char* newValue, enum_field_types newType, uint32 length);

    void CleanUp()
    {
        delete[]((char*)data.value);
        data.value = nullptr;
    }

    static size_t SizeForType(MYSQL_FIELD* field)
    {
        switch (field->type)
        {
        case MYSQL_TYPE_NULL:
            return 0;
        case MYSQL_TYPE_TINY:
            return 1;
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_SHORT:
            return 2;
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
            return 4;
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_BIT:
            return 8;

        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATETIME:
            return sizeof(MYSQL_TIME);

        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
            return field->max_length + 1;

        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            return 64;

        case MYSQL_TYPE_GEOMETRY:
            /*
            Following types are not sent over the wire:
            MYSQL_TYPE_ENUM:
            MYSQL_TYPE_SET:
            */
        default:
            SF_LOG_WARN("sql.sql", "SQL::SizeForType(): invalid field type %u", uint32(field->type));
            return 0;
        }
    }

    [[nodiscard]] bool IsType(enum_field_types type) const
    {
        return data.type == type;
    }

    [[nodiscard]] bool IsNumeric() const
    {
        return (data.type == MYSQL_TYPE_TINY ||
            data.type == MYSQL_TYPE_SHORT ||
            data.type == MYSQL_TYPE_INT24 ||
            data.type == MYSQL_TYPE_LONG ||
            data.type == MYSQL_TYPE_FLOAT ||
            data.type == MYSQL_TYPE_DOUBLE ||
            data.type == MYSQL_TYPE_LONGLONG);
    }

    [[nodiscard]] bool IsBinary() const
    {
        return (
            data.type == MYSQL_TYPE_TINY_BLOB ||
            data.type == MYSQL_TYPE_MEDIUM_BLOB ||
            data.type == MYSQL_TYPE_LONG_BLOB ||
            data.type == MYSQL_TYPE_BLOB ||
            data.type == MYSQL_TYPE_VAR_STRING ||
            data.type == MYSQL_TYPE_STRING
            );
    }

    void GetBinarySizeChecked(uint8* buf, size_t size) const;

private:
#ifdef SKYFIRE_DEBUG
    static char const* FieldTypeToString(enum_field_types type)
    {
        switch (type)
        {
        case MYSQL_TYPE_BIT:
            return "BIT";
        case MYSQL_TYPE_BLOB:
            return "BLOB";
        case MYSQL_TYPE_DATE:
            return "DATE";
        case MYSQL_TYPE_DATETIME:
            return "DATETIME";
        case MYSQL_TYPE_NEWDECIMAL:
            return "NEWDECIMAL";
        case MYSQL_TYPE_DECIMAL:
            return "DECIMAL";
        case MYSQL_TYPE_DOUBLE:
            return "DOUBLE";
        case MYSQL_TYPE_ENUM:
            return "ENUM";
        case MYSQL_TYPE_FLOAT:
            return "FLOAT";
        case MYSQL_TYPE_GEOMETRY:
            return "GEOMETRY";
        case MYSQL_TYPE_INT24:
            return "INT24";
        case MYSQL_TYPE_LONG:
            return "LONG";
        case MYSQL_TYPE_LONGLONG:
            return "LONGLONG";
        case MYSQL_TYPE_LONG_BLOB:
            return "LONG_BLOB";
        case MYSQL_TYPE_MEDIUM_BLOB:
            return "MEDIUM_BLOB";
        case MYSQL_TYPE_NEWDATE:
            return "NEWDATE";
        case MYSQL_TYPE_NULL:
            return "nullptr";
        case MYSQL_TYPE_SET:
            return "SET";
        case MYSQL_TYPE_SHORT:
            return "SHORT";
        case MYSQL_TYPE_STRING:
            return "STRING";
        case MYSQL_TYPE_TIME:
            return "TIME";
        case MYSQL_TYPE_TIMESTAMP:
            return "TIMESTAMP";
        case MYSQL_TYPE_TINY:
            return "TINY";
        case MYSQL_TYPE_TINY_BLOB:
            return "TINY_BLOB";
        case MYSQL_TYPE_VAR_STRING:
            return "VAR_STRING";
        case MYSQL_TYPE_YEAR:
            return "YEAR";
        default:
            return "-Unknown-";
        }
    }
#endif
};

#endif
