#include "Sql.h"

Sql::Sql()
{
    this->space_ = 0;
    this->table_quote_char_ = 0;
}

Sql::~Sql()
{
    if (this->space_)
        delete[] this->space_;

    if (this->table_quote_char_)
        delete[] this->table_quote_char_;
}

void Sql::set_cache(Cache *cache)
{
    this->cache = cache;
}

void Sql::set_space(TCHAR *space)
{
    unsigned long len = _tcslen(space) + 1;
    this->space_ = new TCHAR[len];
    memset(this->space_, 0, len);
    _tcscpy_s(this->space_, len, space);
}

void Sql::set_table_quote_char(TCHAR *table_quote_char)
{
    unsigned long len = _tcslen(table_quote_char) + 1;
    this->table_quote_char_ = new TCHAR[len];
    memset(this->table_quote_char_, 0, len);
    _tcscpy_s(this->table_quote_char_, len, table_quote_char);
}

TCHAR *Sql::space() const
{
    return this->space_;
}

TCHAR *Sql::table_quote_char() const
{
    return this->table_quote_char_;
}

/// <summary>
/// Gets the database version.
/// </summary>
/// <returns></returns>
long Sql::GetDatabaseVersion() const
{
    return RESULT_NOT_FOUND;
}

long Sql::GetDatabaseCount() const
{
    return RESULT_NOT_FOUND;
}

TCHAR *Sql::GetDatabaseName(long id) const
{
    return 0;
}

long Sql::GetTableCount(TCHAR *dbname) const
{
    return RESULT_NOT_FOUND;
}

TCHAR *Sql::GetTableName(TCHAR *dbname, long id) const
{
    return 0;
}

long Sql::GetColumnCount(TCHAR *dbname, TCHAR *tablename) const
{
    return RESULT_NOT_FOUND;
}

TCHAR *Sql::GetColumnName(TCHAR *dbname, TCHAR *tablename, long id) const
{
    return 0;
}

long Sql::GetRowCount(TCHAR *dbname, TCHAR *tablename) const
{
    return RESULT_NOT_FOUND;
}

TCHAR *Sql::GetRowData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long row) const
{
    return 0;
}

std::map<TCHAR*, TCHAR*> Sql::GetRowData(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnnames, long row) const
{
    std::map<TCHAR*, TCHAR*> result;
    return result;
}

TCHAR *Sql::GetCustomData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *tarGet, TCHAR *criteria) const
{
    return 0;
}

TCHAR *Sql::LoadFile(TCHAR* filePath) const
{
    return 0;
}

TCHAR *Sql::GetConfigValue(TCHAR* variable) const
{
    return 0;
}

TCHAR *Sql::ExecuteFunction(TCHAR* function) const
{
    return 0;
}

void Sql::SaveFile(TCHAR* filepath, TCHAR* contents, size_t contents_size_bytes) const
{
}
