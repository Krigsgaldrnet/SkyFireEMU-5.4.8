/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include <ace/Activation_Queue.h>

#include "DatabaseWorkerPool.h"
#include "Transaction.h"
#include "Util.h"

#ifndef _MYSQLCONNECTION_H
#define _MYSQLCONNECTION_H

class DatabaseWorker;
class PreparedStatement;
class MySQLPreparedStatement;
class PingOperation;

enum ConnectionFlags
{
    CONNECTION_ASYNC = 0x1,
    CONNECTION_SYNCH = 0x2,
    CONNECTION_BOTH = CONNECTION_ASYNC | CONNECTION_SYNCH
};

struct MySQLConnectionInfo
{
    explicit MySQLConnectionInfo(const char* host, const char* port, const char* user, const char* password, const char* database)
    {
        _host.assign(host);
        _port_or_socket.assign(port);
        _user.assign(user);
        _password.assign(password);
        _database.assign(database);
    }

    explicit MySQLConnectionInfo(std::string const& infoString)
    {
        Tokenizer tokens(infoString, ';');

        if (tokens.size() != 5)
            return;

        uint8 i = 0;

        _host.assign(tokens[i++]);
        _port_or_socket.assign(tokens[i++]);
        _user.assign(tokens[i++]);
        _password.assign(tokens[i++]);
        _database.assign(tokens[i++]);
    }

    std::string _user;
    std::string _password;
    std::string _database;
    std::string _host;
    std::string _port_or_socket;
};

typedef std::map<uint32 /*index*/, std::pair<std::string /*query*/, ConnectionFlags /*sync/async*/> > PreparedStatementMap;

class MySQLConnection
{
    template <class T> friend class DatabaseWorkerPool;
    friend class PingOperation;

public:
    MySQLConnection(MySQLConnectionInfo& connInfo);                               //! Constructor for synchronous connections.
    MySQLConnection(ACE_Activation_Queue* queue, MySQLConnectionInfo& connInfo);  //! Constructor for asynchronous connections.
    virtual ~MySQLConnection();

    virtual bool Open();
    void Close();

public:
    bool Execute(const char* sql);
    bool Execute(PreparedStatement* stmt);
    ResultSet* Query(const char* sql);
    PreparedResultSet* Query(PreparedStatement* stmt);
    bool _Query(const char* sql, MYSQL_RES** pResult, MYSQL_FIELD** pFields, uint64* pRowCount, uint32* pFieldCount);
    bool _Query(PreparedStatement* stmt, MYSQL_RES** pResult, uint64* pRowCount, uint32* pFieldCount);

    void BeginTransaction();
    void RollbackTransaction();
    void CommitTransaction();
    bool ExecuteTransaction(SQLTransaction& transaction);

    operator bool() const { return m_Mysql != NULL; }
    void Ping() { mysql_ping(m_Mysql); }

    uint32 GetLastError() { return mysql_errno(m_Mysql); }

protected:
    bool LockIfReady()
    {
        /// Tries to acquire lock. If lock is acquired by another thread
        /// the calling parent will just try another connection
        return m_Mutex.tryacquire() != -1;
    }

    void Unlock()
    {
        /// Called by parent databasepool. Will let other threads access this connection
        m_Mutex.release();
    }

    MYSQL* GetHandle() { return m_Mysql; }
    MySQLPreparedStatement* GetPreparedStatement(uint32 index);
    void PrepareStatement(uint32 index, std::string sql, ConnectionFlags flags);

    bool PrepareStatements();
    virtual void DoPrepareStatements() = 0;

protected:
    std::vector<MySQLPreparedStatement*> m_stmts;         //! PreparedStatements storage
    PreparedStatementMap                 m_queries;       //! Query storage
    bool                                 m_reconnecting;  //! Are we reconnecting?
    bool                                 m_prepareError;  //! Was there any error while preparing statements?

private:
    bool _HandleMySQLErrno(uint32 errNo);

private:
    ACE_Activation_Queue* m_queue;                      //! Queue shared with other asynchronous connections.
    DatabaseWorker* m_worker;                     //! Core worker task.
    MYSQL* m_Mysql;                      //! MySQL Handle.
    MySQLConnectionInfo& m_connectionInfo;             //! Connection info (used for logging)
    ConnectionFlags       m_connectionFlags;            //! Connection flags (for preparing relevant statements)
    ACE_Thread_Mutex      m_Mutex;

    MySQLConnection(MySQLConnection const& right) = delete;
    MySQLConnection& operator=(MySQLConnection const& right) = delete;
};

#endif
