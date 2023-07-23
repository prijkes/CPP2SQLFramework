#ifndef _MYSQL_H_
#define _MYSQL_H_

#include "sql.h"
#include "strings.h"

#define INT_REVERSE(x) (((x)>>24)&0xFF)| \
((((x)>>16)&0xFF)<<8)| \
((((x)>>8)&0xFF)<<16)| \
((((x)>>0)&0xFF)<<24)

// PCRE
#ifdef USE_PCRE_REGEX
#include <pcre.h>
#pragma comment(lib, "pcre")
#endif

class Mysql final : public Sql
{
private:
    //
    // Functions
    //
    TCHAR *Mysql::GetHttpData(TCHAR *query) const;
    long SendQuery(TCHAR *query) const;

    long BruteforceQuery(TCHAR *sub_qry, unsigned short start, unsigned short end) const;
    long CacheQuery(TCHAR *qry, bool use_charset) const;

    TCHAR *UnionSelectQuery(TCHAR *qry, bool use_charset = false, unsigned short param1 = 0, unsigned short param2 = 0) const;

    long GetCharResult(TCHAR *qry, bool use_charset = false, unsigned short param1 = 0, unsigned short param2 = 0, bool brute_use_binary = true) const;
    TCHAR *GetLengthBasedResult(TCHAR *qry, bool use_charset = false, unsigned short param1 = 0, unsigned short param2 = 0, bool count_length = true) const;
    std::map<TCHAR*, TCHAR*> Mysql::GetSetResult(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnnames, long row) const;

    // mysql injection data
    TCHAR *base36charset = _T("0123456789abcdefghijklmnopqrstuvwxyz");
    TCHAR *extended_charset = _T("_-@./: !#$%&'*+=?^`{|}~");
    TCHAR *ascii_charset1 = _T(" !\"#$%&'()*+,-./");    // 0x20 - 0x2E
    TCHAR *ascii_charset2 = _T(":;<=>?@");              // 0x3A - 0x40
    TCHAR *ascii_charset3 = _T("[\\]^_`");               // 0x5B - 0x60
    TCHAR *ascii_charset4 = _T("{|}~");                 // 0x7B - 0x7E
    TCHAR *ascii_full_charset = _T(" !\"#$%&'()*+,-./0123456789:; <= > ? @[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
    TCHAR *table_charset = _T("0123456789abcdefghijklmnopqrstuvwxyz[]_-@.");

    TCHAR *common_tables;       // common_tables? // not implented yet

    mutable unsigned long requests_;

public:
    Mysql(Methods method, TCHAR *host, unsigned short port, TCHAR *path, TCHAR *rest);
    ~Mysql();

    // host
    TCHAR *host;                // Host string
    TCHAR *ip;                  // IP of host
    char *_ip;                  // IP in MBCS aka. normal char[]
    unsigned short port;        // Port
    mutable TCHAR *path;        // Path to vulnerable part
    TCHAR *rest;                // Part after vulnerable part

#ifdef USE_PCRE_REGEX
    pcre *re;                   // Regular expression struct
    char *regexp;               // Regular expression holder
#else
    TCHAR *regexp;              // Search string holder
#endif

    long GetDatabaseVersion() const override;
    long GetDatabaseCount() const override;
    TCHAR *GetDatabaseName(long id) const override;
    long GetTableCount(TCHAR *dbname) const override;
    TCHAR *GetTableName(TCHAR *dbname, long id) const override;
    long GetColumnCount(TCHAR *dbname, TCHAR *tablename) const override;
    TCHAR *GetColumnName(TCHAR *dbname, TCHAR *tablename, long id) const override;
    long GetRowCount(TCHAR *dbname, TCHAR *tablename) const override;
    TCHAR *GetRowData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long row) const override;
    std::map<TCHAR*, TCHAR*> GetRowData(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnnames, long row) const override;
    TCHAR *GetCustomData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *target, TCHAR *criteria) const override;

    TCHAR *LoadFile(TCHAR *filepath) const override;
    TCHAR *GetConfigValue(TCHAR *variable) const override;
    TCHAR *ExecuteFunction(TCHAR *function) const override;
    void SaveFile(TCHAR *filepath, TCHAR *contents, size_t contents_size_bytes) const override;
    void SaveFile(TCHAR* filepath, TCHAR *contents, size_t contents_size_bytes, bool use_decompression) const;

    bool IsKnownDatabase(TCHAR *dbname) const;

    unsigned long requests() const { return this->requests_; }

    void SetPath(TCHAR *path, TCHAR *rest);
};

#endif
