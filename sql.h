#ifndef _SQL_H_
#define _SQL_H_

#include <tchar.h>
#include <wchar.h>
#include <string.h>
#include <math.h>
#include <map>

#include "http.h"
#include "cache.h"
#include "history.h"
#include "logger.h"

#ifdef RESULT_NOT_FOUND
#undef RESULT_NOT_FOUND
#endif
#define RESULT_NOT_FOUND -1

/// <summary>
/// Methods to use for enumerating informating.
/// </summary>
enum Methods
{
    kMethodHistory,           // Not implemented
    kMethodBruteforce,
    kMethodCachingNumberic,
    kMethodCachingStrings,
    kMethodUnionSelect,
};

class Sql
{
public:
    const TCHAR *charset = _T("0123456789abcdefghijklmnopqrstuvwxyz_-@./: !#$%&'*+=()?^`'{|},[]<>\"\\~\r\n\t\x79"); // Charset to use
    const unsigned long charlen = _tcslen(charset); // Length of charsets

    const TCHAR *union_param_key = _T("XxxX");

    void set_cache(Cache *cache);
    void set_space(TCHAR *space);
    void set_table_quote_char(TCHAR *table_quote_char);

    TCHAR *space() const;
    TCHAR *table_quote_char() const;

    virtual long GetDatabaseVersion() const;
    virtual long GetDatabaseCount() const;
    virtual TCHAR *GetDatabaseName(long id) const;
    virtual long GetTableCount(TCHAR *dbname) const;
    virtual TCHAR *GetTableName(TCHAR *dbname, long id) const;
    virtual long GetColumnCount(TCHAR *dbname, TCHAR *tablename) const;
    virtual TCHAR *GetColumnName(TCHAR *dbname, TCHAR *tablename, long id) const;
    virtual long GetRowCount(TCHAR *dbname, TCHAR *tablename) const;
    virtual TCHAR *GetRowData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long row) const;
    virtual std::map<TCHAR*, TCHAR*> GetRowData(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnnames, long row) const;
    virtual TCHAR *GetCustomData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *target, TCHAR *criteria) const;

    virtual TCHAR *LoadFile(TCHAR *filepath) const;
    virtual TCHAR *GetConfigValue(TCHAR *variable) const;
    virtual TCHAR *ExecuteFunction(TCHAR *function) const;
    virtual void SaveFile(TCHAR* filepath, TCHAR* contents, size_t contents_size_bytes) const;

protected:
    Sql();
    ~Sql();

    Http *http = &Http::GetInstance();
    Cache *cache = 0;

    /// <summary>
    /// The space character used in the query URL.
    /// </summary>
    TCHAR *space_;

    TCHAR *table_quote_char_;

    Methods method;
};

#endif
