/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "Log.h"
#include "MySQLConnection.h"
#include "PreparedStatement.h"
#include "QueryHolder.h"
#include "CompilerDefs.h"

bool SQLQueryHolder::SetQuery(size_t index, const char* sql)
{
    if (m_queries.size() <= index)
    {
        SF_LOG_ERROR("sql.sql", "Query index (%u) out of range (size: %u) for query: %s", uint32(index), (uint32)m_queries.size(), sql);
        return false;
    }

    /// not executed yet, just stored (it's not called a holder for nothing)
    SQLElementData element;
    element.type = SQL_ELEMENT_RAW;
    element.element.query = strdup(sql);

    SQLResultSetUnion result;
    result.qresult = NULL;

    m_queries[index] = SQLResultPair(element, result);
    return true;
}

bool SQLQueryHolder::SetPQuery(size_t index, const char* format, ...)
{
    if (!format)
    {
        SF_LOG_ERROR("sql.sql", "Query (index: %u) is empty.", uint32(index));
        return false;
    }

    va_list ap;
    char szQuery[MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf(szQuery, MAX_QUERY_LEN, format, ap);
    va_end(ap);

    if (res == -1)
    {
        SF_LOG_ERROR("sql.sql", "SQL Query truncated (and not execute) for format: %s", format);
        return false;
    }

    return SetQuery(index, szQuery);
}

bool SQLQueryHolder::SetPreparedQuery(size_t index, PreparedStatement* stmt)
{
    if (m_queries.size() <= index)
    {
        SF_LOG_ERROR("sql.sql", "Query index (%u) out of range (size: %u) for prepared statement", uint32(index), (uint32)m_queries.size());
        return false;
    }

    /// not executed yet, just stored (it's not called a holder for nothing)
    SQLElementData element;
    element.type = SQL_ELEMENT_PREPARED;
    element.element.stmt = stmt;

    SQLResultSetUnion result;
    result.presult = NULL;

    m_queries[index] = SQLResultPair(element, result);
    return true;
}

QueryResult SQLQueryHolder::GetResult(size_t index)
{
    // Don't call to this function if the index is of an ad-hoc statement
    if (index < m_queries.size())
    {
        ResultSet* result = m_queries[index].second.qresult;
        if (!result || !result->GetRowCount())
            return QueryResult(NULL);

        result->NextRow();
        return QueryResult(result);
    }
    else
        return QueryResult(NULL);
}

PreparedQueryResult SQLQueryHolder::GetPreparedResult(size_t index)
{
    // Don't call to this function if the index is of a prepared statement
    if (index < m_queries.size())
    {
        PreparedResultSet* result = m_queries[index].second.presult;
        if (!result || !result->GetRowCount())
            return PreparedQueryResult(NULL);

        return PreparedQueryResult(result);
    }
    else
        return PreparedQueryResult(NULL);
}

void SQLQueryHolder::SetResult(size_t index, ResultSet* result)
{
    if (result && !result->GetRowCount())
    {
        delete result;
        result = NULL;
    }

    /// store the result in the holder
    if (index < m_queries.size())
        m_queries[index].second.qresult = result;
}

void SQLQueryHolder::SetPreparedResult(size_t index, PreparedResultSet* result)
{
    if (result && !result->GetRowCount())
    {
        delete result;
        result = NULL;
    }

    /// store the result in the holder
    if (index < m_queries.size())
        m_queries[index].second.presult = result;
}

SQLQueryHolder::~SQLQueryHolder()
{
    for (size_t i = 0; i < m_queries.size(); i++)
    {
        /// if the result was never used, free the resources
        /// results used already (getresult called) are expected to be deleted
        if (SQLElementData* data = &m_queries[i].first)
        {
            switch (data->type)
            {
                case SQL_ELEMENT_RAW:
                    free((void*)(const_cast<char*>(data->element.query)));
                    break;
                case SQL_ELEMENT_PREPARED:
                    delete data->element.stmt;
                    break;
            }
        }
    }
}

void SQLQueryHolder::SetSize(size_t size)
{
    /// to optimize push_back, reserve the number of queries about to be executed
    m_queries.resize(size);
}

bool SQLQueryHolderTask::Execute()
{
    //the result can't be ready as we are processing it right now
    ASSERT(!m_result.ready());

    if (!m_holder)
        return false;

    /// we can do this, we are friends
    std::vector<SQLQueryHolder::SQLResultPair>& queries = m_holder->m_queries;

    for (size_t i = 0; i < queries.size(); i++)
    {
        /// execute all queries in the holder and pass the results
        if (SQLElementData* data = &queries[i].first)
        {
            switch (data->type)
            {
                case SQL_ELEMENT_RAW:
                {
                    char const* sql = data->element.query;
                    if (sql)
                        m_holder->SetResult(i, m_conn->Query(sql));
                    break;
                }
                case SQL_ELEMENT_PREPARED:
                {
                    PreparedStatement* stmt = data->element.stmt;
                    if (stmt)
                        m_holder->SetPreparedResult(i, m_conn->Query(stmt));
                    break;
                }
            }
        }
    }

    m_result.set(m_holder);
    return true;
}
