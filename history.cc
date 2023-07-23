#include "History.h"

History::History()
{
    this->hostcount_ = 0;
}

History::~History()
{
    for (auto host : hosts)
        delete host;
}

HistoryHost *History::GetHost(TCHAR *hostname) const
{
    if (!hostname)
        return 0;

    for (auto host : hosts)
    {
        if (!_tcsicmp(hostname, host->name))
            return host;
    }
    return 0;
}

HistoryHost *History::GetHost(long id) const
{
    if (!id)
        return 0;

    for (auto host : hosts)
    {
        if (host->id == id)
            return host;
    }
    return 0;
}

HistoryDatabase *History::GetDatabase(TCHAR *hostname, TCHAR *dbname) const
{
    if (!dbname)
        return 0;

    HistoryHost *host = this->GetHost(hostname);
    if (host)
    {
        for (auto db : host->databases)
        {
            if (!_tcsicmp(dbname, db->name))
                return db;
        }
    }
    return 0;
}

HistoryDatabase *History::GetDatabase(TCHAR *hostname, long id) const
{
    if (!id)
        return 0;

    HistoryHost *host = this->GetHost(hostname);
    if (host)
    {
        for (auto db : host->databases)
        {
            if (db->id == id)
                return db;
        }
    }
    return 0;
}

HistoryTable *History::GetTable(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename) const
{
    if (!tablename)
        return 0;

    HistoryDatabase *db = this->GetDatabase(hostname, dbname);
    if (db)
    {
        for (auto table : db->tables)
        {
            if (!_tcsicmp(tablename, table->name))
                return table;
        }
    }
    return 0;
}

HistoryTable *History::GetTable(TCHAR *hostname, TCHAR *dbname, long id) const
{
    if (!id)
        return 0;

    HistoryDatabase *db = this->GetDatabase(hostname, dbname);
    if (db)
    {
        for (auto table : db->tables)
        {
            if (table->id == id)
                return table;
        }
    }
    return 0;
}

HistoryColumn *History::GetColumn(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname) const
{
    if (!columnname)
        return 0;

    HistoryTable *table = this->GetTable(hostname, dbname, tablename);
    if (table)
    {
        for (auto column : table->columns)
        {
            if (!_tcsicmp(columnname, column->name))
                return column;
        }
    }
    return 0;
}

HistoryColumn *History::GetColumn(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, long id) const
{
    if (!id)
        return 0;

    HistoryTable *table = this->GetTable(hostname, dbname, tablename);
    if (table)
    {
        for (auto column : table->columns)
        {
            if (column->id == id)
                return column;
        }
    }
    return 0;
}

HistoryRow *History::GetRow(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *rowdata) const
{
    if (!rowdata)
        return 0;

    HistoryColumn *column = this->GetColumn(hostname, dbname, tablename, columnname);
    if (column)
    {
        for (auto row : column->rows)
        {
            if (!_tcsicmp(rowdata, row->data))
                return row;
        }
    }
    return 0;
}

HistoryRow *History::GetRow(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long id) const
{
    if (!id)
        return 0;

    HistoryColumn *column = this->GetColumn(hostname, dbname, tablename, columnname);
    if (column)
    {
        for (auto row : column->rows)
        {
            if (row->id == id)
                return row;
        }
    }
    return 0;
}

HistoryFile *History::GetFile(TCHAR *hostname, TCHAR *filepath) const
{
    if (!filepath)
        return 0;

    HistoryHost *host = this->GetHost(hostname);
    if (host)
    {
        for (auto file : host->files)
        {
            if (!_tcscmp(filepath, file->filepath))
                return file;
        }
    }
    return 0;
}

HistoryVariable *History::GetConfigValue(TCHAR *hostname, TCHAR *variable) const
{
    if (!variable)
        return 0;

    HistoryHost *host = this->GetHost(hostname);
    if (host)
    {
        for (auto pair : host->variables)
        {
            if (!_tcscmp(variable, pair->variable))
                return pair;
        }
    }
    return 0;
}

HistoryHost *History::AddHost(TCHAR *hostname) const
{
    if (!hostname)
        return 0;

    HistoryHost *host = this->GetHost(hostname);
    if (!host)
    {
        host = new HistoryHost;
        host->id = ++this->hostcount_;
        this->hosts.insert(this->hosts.end(), host);
    }
    
    if (host->name != hostname)
    {
        if (host->name)
            delete[] host->name;

        unsigned long len = _tcslen(hostname) + 1;
        host->name = new TCHAR[len];
        memset(host->name, 0, len * sizeof(TCHAR));
        _tcscpy_s(host->name, len, hostname);
    }

    return host;
}

HistoryDatabase *History::AddDatabase(TCHAR *hostname, TCHAR *dbname, long id) const
{
    if (!dbname)
        return 0;

    HistoryHost *host = this->AddHost(hostname);
    HistoryDatabase *db = this->GetDatabase(hostname, dbname);
    if (!db)
    {
        db = new HistoryDatabase;
        host->databases.insert(host->databases.end(), db);
        db->host = host;
    }
    
    if (db->name != dbname)
    {
        if (db->name)
            delete[] db->name;

        unsigned long len = _tcslen(dbname) + 1;
        db->name = new TCHAR[len];
        memset(db->name, 0, len * sizeof(TCHAR));
        _tcscpy_s(db->name, len, dbname);
    }

    if (db->id == 0 && id != 0)
        db->id = id;

    return db;
}

HistoryTable *History::AddTable(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, long id) const
{
    if (!tablename)
        return 0;

    HistoryDatabase *db = this->AddDatabase(hostname, dbname, 0);
    HistoryTable *table = this->GetTable(hostname, dbname, tablename);
    if (!table)
    {
        table = new HistoryTable;
        db->tables.insert(db->tables.end(), table);
        table->db = db;
    }
    
    if (table->name != tablename)
    {
        if (table->name)
            delete[] table->name;

        unsigned long len = _tcslen(tablename) + 1;
        table->name = new TCHAR[len];
        memset(table->name, 0, len * sizeof(TCHAR));
        _tcscpy_s(table->name, len, tablename);
    }

    if (table->id == 0 && id != 0)
        table->id = id;

    return table;
}

HistoryColumn *History::AddColumn(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long id) const
{
    if (!columnname)
        return 0;

    HistoryTable *table = this->AddTable(hostname, dbname, tablename, 0);
    HistoryColumn *column = this->GetColumn(hostname, dbname, tablename, columnname);
    if (!column)
    {
        column = new HistoryColumn;
        table->columns.insert(table->columns.end(), column);
        column->table = table;
    }
    
    if (column->name != columnname)
    {
        if (column->name)
            delete[] column->name;

        unsigned len = _tcslen(columnname) + 1;
        column->name = new TCHAR[len];
        memset(column->name, 0, len * sizeof(TCHAR));
        _tcscpy_s(column->name, len, columnname);
    }
    
    if (column->id == 0 && id != 0)
        column->id = id;

    return column;
}

HistoryRow *History::AddRow(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *rowdata, long id) const
{
    if (!rowdata)
        return 0;

    HistoryColumn *column = this->AddColumn(hostname, dbname, tablename, columnname, 0);
    HistoryRow *row = this->GetRow(hostname, dbname, tablename, columnname, rowdata);
    if (!row || row->id != id)
    {
        row = new HistoryRow;
        column->rows.insert(column->rows.end(), row);
        row->column = column;
    }
    
    if (row->data != rowdata)
    {
        if (row->data)
            delete[] row->data;

        row->length = _tcslen(rowdata);
        unsigned long len = row->length + 1;
        row->data = new TCHAR[len];
        memset(row->data, 0, len * sizeof(TCHAR));
        _tcscpy_s(row->data, len, rowdata);
    }
        
    if (row->id == 0 && id != 0)
        row->id = id;

    return row;
}

HistoryFile *History::AddFile(TCHAR *hostname, TCHAR *filepath, TCHAR *contents) const
{
    HistoryHost *host = this->AddHost(hostname);
    HistoryFile *file = this->GetFile(hostname, filepath);
    if (file)
        return file;

    file = new HistoryFile();
    file->host = host;

    size_t f_len = _tcslen(filepath);
    file->filepath = new TCHAR[f_len + 1];
    memset(file->filepath, 0, (f_len + 1) * sizeof(TCHAR));
    memcpy(file->filepath, filepath, f_len * sizeof(TCHAR));

    if (contents)
    {
        size_t c_len = _tcslen(contents);
        file->contents = new TCHAR[c_len + 1];
        memset(file->contents, 0, (c_len + 1) * sizeof(TCHAR));
        memcpy(file->contents, contents, c_len * sizeof(TCHAR));
    }
    host->files.insert(host->files.end(), file);
    return file;
}

HistoryVariable *History::AddConfigValue(TCHAR *hostname, TCHAR *variable, TCHAR *value) const
{
    if (!value)
        return 0;

    HistoryHost *host = this->AddHost(hostname);
    HistoryVariable *pair = this->GetConfigValue(hostname, variable);
    if (pair)
        return pair;

    pair = new HistoryVariable();
    pair->host = host;

    size_t var_len = _tcslen(variable), val_len = _tcslen(value);
    pair->variable = new TCHAR[var_len + 1];
    memset(pair->variable, 0, (var_len + 1) * sizeof(TCHAR));
    memcpy(pair->variable, variable, var_len * sizeof(TCHAR));

    pair->value = new TCHAR[val_len + 1];
    memset(pair->value, 0, (val_len + 1) * sizeof(TCHAR));
    memcpy(pair->value, value, val_len * sizeof(TCHAR));

    host->variables.insert(host->variables.end(), pair);
    return pair;
}
