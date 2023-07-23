#ifndef _HISTORY_H_
#define _HISTORY_H_
/*
    This file only contains the structs for the history variable.
    These structs keeps track of what has been found internally.
*/
#include <tchar.h>
#include <string.h>
#include <vector>

#ifdef RESULT_NOT_FOUND
#undef RESULT_NOT_FOUND
#endif
#define RESULT_NOT_FOUND -1

struct HistoryHost;
struct HistoryDatabase;
struct HistoryTable;
struct HistoryColumn;
struct HistoryRow;

struct HistoryVariable
{
    HistoryVariable()
    {
        this->variable = 0;
        this->value = 0;
        this->host = 0;
    }

    ~HistoryVariable()
    {
        if (this->variable)
            delete[] this->variable;

        if (this->value)
            delete[] this->value;
    }

    TCHAR *variable;
    TCHAR *value;
    HistoryHost *host;
};

struct HistoryFile
{
    HistoryFile()
    {
        this->filepath = 0;
        this->contents = 0;
        this->host = 0;
    }

    ~HistoryFile()
    {
        if (this->filepath)
            delete[] this->filepath;

        if (this->contents)
            delete[] this->contents;
    }

    TCHAR *filepath;
    TCHAR *contents;
    HistoryHost *host;
};

struct HistoryRow
{
    HistoryRow()
    {
        this->id = 0;
        this->length = 0;
        this->data = 0;
        this->column = 0;
    }

    ~HistoryRow()
    {
        if (this->data)
            delete[] this->data;
    }

    long id;
    long length;
    TCHAR *data;
    HistoryColumn* column;
};

struct HistoryColumn
{
    HistoryColumn()
    {
        this->id = 0;
        this->name = 0;
        this->data_type = 0;
        this->length = 0;
        this->allow_null = true;
        this->auto_increment = false;
        this->default_value = 0;
        this->table = 0;

        for (auto row : rows)
            delete row;
    }

    ~HistoryColumn()
    {
        if (this->name)
            delete[] this->name;

        if (this->data_type)
            delete[] this->data_type;

        if (this->default_value)
            delete[] this->default_value;
    }

    long id;
    TCHAR *name;
    TCHAR *data_type;
    long length; // data type length, ie. CHAR(255) or INT(10)
    bool allow_null;
    bool auto_increment;
    TCHAR *default_value;
    std::vector<HistoryRow*> rows;
    HistoryTable *table;
};

struct HistoryTable
{
    HistoryTable()
    {
        this->id = 0;
        this->name = 0;
        this->columncount = RESULT_NOT_FOUND;
        this->rowcount = RESULT_NOT_FOUND;
        this->db = 0;
    }

    ~HistoryTable()
    {
        if (this->name)
            delete[] this->name;

        for (auto column : columns)
            delete column;
    }

    long id;
    TCHAR *name;
    long columncount;
    long rowcount;
    std::vector<HistoryColumn*> columns;
    HistoryDatabase *db;
};

struct HistoryDatabase
{
    HistoryDatabase()
    {
        this->id = 0;
        this->name = 0;
        this->character_set = 0;
        this->collation = 0;
        this->tablecount = RESULT_NOT_FOUND;
        this->host = 0;
    }

    ~HistoryDatabase()
    {
        if (this->name)
            delete[] this->name;

        if (this->character_set)
            delete[] this->character_set;

        if (this->collation)
            delete[] this->collation;

        for (auto table : tables)
            delete table;
    }

    long id;
    TCHAR *name;
    TCHAR *character_set;
    TCHAR *collation;
    long tablecount;
    std::vector<HistoryTable*> tables;
    HistoryHost *host;
};

struct HistoryHost
{
    HistoryHost()
    {
        this->id = 0;
        this->name = 0;
        this->version = 0;
        this->databasecount = RESULT_NOT_FOUND;
    }

    ~HistoryHost()
    {
        if (this->name)
            delete[] this->name;

        for (auto database : databases)
            delete database;

        for (auto variable : variables)
            delete variable;

        for (auto file : files)
            delete file;
    }

    long id;
    TCHAR *name;
    long version;
    long databasecount;
    std::vector<HistoryDatabase*> databases;

    std::vector<HistoryFile*> files;
    std::vector<HistoryVariable*> variables;
};

class History
{
private:
    mutable long hostcount_;
    mutable std::vector<HistoryHost*> hosts;

public:
    History();
    ~History();

    HistoryHost *AddHost(TCHAR *hostname) const;
    HistoryDatabase *AddDatabase(TCHAR *hostname, TCHAR *dbname, long id) const;
    HistoryTable *AddTable(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, long id) const;
    HistoryColumn *AddColumn(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long id) const;
    HistoryRow *AddRow(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *rowdata, long id) const;

    HistoryFile *AddFile(TCHAR *hostname, TCHAR *filepath, TCHAR *contents) const;
    HistoryVariable *AddConfigValue(TCHAR *hostname, TCHAR *variable, TCHAR *value) const;

    long hostcount() const { return this->hostcount_; }

    // id's are 1-based, array index are 0-based
    HistoryHost *GetHost(long id) const;
    HistoryHost *GetHost(TCHAR* hostname) const;
    HistoryDatabase *GetDatabase(TCHAR *hostname, TCHAR *dbname) const;
    HistoryDatabase *GetDatabase(TCHAR *hostname, long id) const;
    HistoryTable *GetTable(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename) const;
    HistoryTable *GetTable(TCHAR *hostname, TCHAR *dbname, long id) const;
    HistoryColumn *GetColumn(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname) const;
    HistoryColumn *GetColumn(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, long id) const;
    HistoryRow *GetRow(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, TCHAR *rowdata) const;
    HistoryRow *GetRow(TCHAR *hostname, TCHAR *dbname, TCHAR *tablename, TCHAR *columnname, long id) const;

    HistoryFile *GetFile(TCHAR *hostname, TCHAR *filepath) const;
    HistoryVariable *GetConfigValue(TCHAR *hostname, TCHAR *variable) const;
};

#endif
