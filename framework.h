#ifndef _FRAMEWORK_H_
#define _FRAMEWORK_H_

#define FRAMEWORK_AUTHOR _T("prijkes")
#define FRAMEWORK_VERSION _T("2.1")

//
#include <stdio.h>
#include <tchar.h>
#include <wchar.h>

#include <algorithm>

//
#include "mysql.h"
#include "mysql_writer.h"
#include "cache.h"
#include "history.h"
#include "logger.h"
#include "strings.h"

enum AttackTypes
{
    // SQL
    kGetMysqlVersion = 1,
    // Databases
    kCountDatabases,
    kGetDatabaseName,
    // Tables
    kCountTablesInDatabase,
    kGetTableNameInDatabase,
    // Columns
    kCountColumnsInTable,
    kGetColumnNameInTable,
    // Rows
    kCountRowsInTable,
    // Data column
    kGetRowDataInColumn,
    // Data table
    kGetRowDataInTable,
    // All
    kDatabaseDoAll,
    // Custom query
    kRunCustomCriteria,

    // File
    kShowFileContents,

    // Config Variables
    kShowConfigVariable,

    // Functions
    kExecuteFunction,

    // Shell
    kUploadShell
};

class Framework
{
private:
    //
    // Variables
    //
    // common
    Http *http;
    Mysql *mysql;
    History history;                // History manager
    Cache *cache;
    TCHAR *error_;                  // Error string

    // host
    TCHAR *host_;                   // Host string
    TCHAR *ip_;                     // IP of host
    char *_ip;                      // IP in MBCS aka. normal char[]
    unsigned short port_;           // Port
    TCHAR *path_;                   // Path to vulnerable part
    TCHAR *rest_;                   // Part after vulnerable part
    bool always_lookup_;            // Always send queries even when result has been found already
    int max_rows_;                  // Max rows to retrieve for each table, if available

    // method
    Methods method_;                // Method
    TCHAR **method_params;          // Parameters to method (if needed)

    // attack info
    AttackTypes attack_type_;       // attack type
    std::vector<TCHAR*> attack_params;          // parameters for attacking

    // internal info
    unsigned long total_requests;   // total amount of requests generated
    unsigned long requests;         // amount of requests generated for one attack

    void set_error(TCHAR *error);
    void WriteSql();
    
    void WriteDatabase(int dbid, TCHAR *dbname, int tableid, TCHAR *tablename, int columnid, TCHAR *columnname, int rowid);
    void WriteTable(TCHAR *dbname, int tableid, TCHAR *tablename, int columnid, TCHAR *columnname, int rowid);
    void WriteColumn(TCHAR *dbname, TCHAR *tablename, int columnid, TCHAR *columnname, int rowid);
    void WriteRow(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, int rowid);

    // SQL
    long GetDatabaseVersion() const;
    long GetDatabaseCount() const;
    TCHAR *GetDatabaseName(long id) const;
    long GetDatabaseId(TCHAR *dbname) const;
    long GetTableCount(TCHAR *dbname) const ;
    TCHAR *GetTableName(TCHAR *dbname, long id) const;
    long GetTableId(TCHAR *dbname, TCHAR *tablename) const;
    long GetColumnCount(TCHAR *dbname, TCHAR *tablename) const;
    TCHAR *GetColumnName(TCHAR *dbname, TCHAR *tablename, long id) const;
    long GetColumnId(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname) const;
    long GetRowCount(TCHAR *dbname, TCHAR *tablename) const;
    TCHAR *GetRowData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long row) const;
    std::map<TCHAR*, TCHAR*> GetRowData(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnname, long row) const;
    TCHAR *GetCustomData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *target, TCHAR *criteria) const;

    // Filesystem
    TCHAR *LoadFile(TCHAR *filepath) const;

    // Config file
    TCHAR *GetConfigValue(TCHAR *variable) const;

    // Other
    TCHAR *ExecuteFunction(TCHAR *function) const;
    void UploadShell(TCHAR *path);

public:
    Framework();
    ~Framework();

    TCHAR *error() const { return this->error_; }
    TCHAR *host() const { return this->host_; }
    TCHAR *ip() const { return this->ip_; }
    unsigned short port() const { return this->port_; }
    TCHAR *path() const { return this->path_; }            // TODO: path is not in unicode - should be in hex %C41F for example - convert it back to unicode and return
    TCHAR *rest() const { return this->rest_; }            // TODO: path is not in unicode - should be in hex %C41F for example - convert it back to unicode and return
    bool always_lookup() const { return this->always_lookup_; }
    int max_rows() const { return this->max_rows_; }
    Methods method() const { return this->method_; }
    unsigned long interval() const { return http->interval; }
    AttackTypes attack_type() const { return this->attack_type_; }
    TCHAR *dynamics_start() const { return http->dynamic_start; }
    TCHAR *dynamic_end() const { return http->dynamic_end; }

    int GetDebug() const { return Logger::GetInstance().GetDebug(); }
    std::wofstream& GetFileHandle() const { return *Logger::GetInstance().GetFileHandle(); }

    bool SetHost(TCHAR *host, unsigned short port = 80);
    bool SetPath(TCHAR *path, TCHAR *rest = 0);
    void SetInterval(unsigned long interval);
    bool SetMethod(Methods method, TCHAR **argv, int argc);
    void SetCookie(TCHAR *cookie);
    void SetSpace(TCHAR *space);
    void SetTableQuoteChar(TCHAR *table_quote_char);
    void SetAttackType(AttackTypes type);
    void SetDebug(int debug) { Logger::GetInstance().SetDebug(debug); }
    void SetDynamicTags(TCHAR *start_tag, TCHAR *end_tag);
    void SetLookup(bool always_lookup);
    void SetMaxRows(int max_rows);

    bool ValidateConfig();            // Checks if needed variables are set

    bool Start(TCHAR **params = 0, int argc = 0);

    void ShowCacheList();     // Print out cache info
    void ShowHistoryList();   // Print out history info
};

#endif
