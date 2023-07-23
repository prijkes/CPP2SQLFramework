#include "mysql_writer.h"

MysqlWriter::MysqlWriter()
{
}

MysqlWriter::~MysqlWriter()
{
}

void MysqlWriter::WriteCons(HistoryTable *table)
{
    if (!table)
        return;

    LogConsole *console = new LogConsole();

    /*
    +----+----------+-----------+
    | id | username | password  |
    +----+----------+-----------+
    |  1 | asdfg    | passwor2d |
    | .. | ........ | ......... |
    | 28 | user23   | user23    |
    +----+----------+-----------+
    28 rows in set (0.00 sec)
    */
    int column_count = table->columns.size();
    size_t *max_columns_length = new size_t[column_count];
    memset(max_columns_length, 0, column_count * sizeof(int));

    int e = 0;
    for (auto column : table->columns)
    {
        max_columns_length[e] = _tcslen(column->name);
        for (auto r : column->rows)
        {
            size_t datalen = _tcslen(r->data);
            if (datalen > max_columns_length[e])
                max_columns_length[e] = datalen;
        }
        max_columns_length[e++] += 2;
    }

    for (int i = 0; i < 3; i++)
    {
        e = 0;
        if (i != 1)
        {
            for (auto column : table->columns)
            {
                console->Write(_T("+"));
                size_t maxlen = max_columns_length[e++];
                for (unsigned int f = 0; f < maxlen; f++)
                    console->Write(_T("-"));
            }
            console->WriteLine(_T("+"));
        }
        else
        {
            console->Write(_T("|"));
            for (auto column : table->columns)
            {
                console->Write(_T(" "));
                console->Write(column->name);
                size_t maxlen = max_columns_length[e++] - _tcslen(column->name) - 2;
                for (unsigned l = 0; l < maxlen; l++)
                    console->Write(_T(" "));

                console->Write(_T(" |"));
            }
            console->WriteLine(_T(""));
        }
    }

    RowColumnsPairMap rows = GetRowColumns(table);
    if (rows.size())
    {
        int row_count = 0;
        for (auto row : rows)
        {
            console->Write(_T("|"));
            std::vector<RowColumnPair> row_column = row.second;
            e = 0;
            for (auto column : table->columns)
            {
                if (e)
                    console->Write(_T(" |"));

                TCHAR *data = _T("NULL");
                auto search = std::find_if(row_column.begin(), row_column.end(), [column](RowColumnPair& pair) { return pair.first == column; });
                if (search != row_column.end())
                    data = search->second->data;
            
                console->Write(_T(" "));
                console->Write(_T("%s"), data);
                size_t datalen = _tcslen(data);
                size_t maxlen = max_columns_length[e++] - datalen - 2;
                while (maxlen--)
                    console->Write(_T(" "));
            }
            console->WriteLine(_T(" |"));
            row_count++;
        }

        e = 0;
        for (auto column : table->columns)
        {
            console->Write(_T("+"));
            size_t maxlen = max_columns_length[e++];
            for (unsigned int f = 0; f < maxlen; f++)
                console->Write(_T("-"));
        }
        console->WriteLine(_T("+"));
        console->WriteLine(_T("%d row%c in set"), rows.size(), rows.size() > 1 ? _T('s') : _T(''));
    }
    else
    {
        console->WriteLine(_T("Empty set"));
    }
    delete[] max_columns_length;
}

void MysqlWriter::WriteFile(TCHAR *filename, HistoryDatabase *db)
{
    if (!filename || !db)
        return;

    LogFile *logfile = new LogFile(filename);
        
    // http://dev.mysql.com/doc/refman/5.0/en/create-database.html
    /*
    CREATE {DATABASE | SCHEMA} [IF NOT EXISTS] db_name
        [create_specification] ...

    create_specification:
        [DEFAULT] CHARACTER SET [=] charset_name
        | [DEFAULT] COLLATE [=] collation_name
    */
    logfile->WriteLine(_T("CREATE DATABASE IF NOT EXISTS %s\r\n    CHARACTER SET = %s\r\n    COLLATE = %s;"),
        db->name, db->character_set ? db->character_set :_T("DEFAULT"), db->collation ? db->collation : _T("DEFAULT"));
    if (db->tables.size())
    {
        logfile->WriteLine(_T("\r\nUSE %s;"), db->name);
        for (auto table : db->tables)
        {
            // http://dev.mysql.com/doc/refman/5.1/en/create-table.html 
            /*
            CREATE [TEMPORARY] TABLE [IF NOT EXISTS] tbl_name
                [(create_definition,...)]
                [table_options]
                [partition_options]
                select_statement
            */
            logfile->WriteLine(_T("\r\nCREATE TABLE IF NOT EXISTS %s"), table->name);
            if (table->columns.size())
            {
                logfile->WriteLine(_T("("));
                size_t c = 0;
                for (auto column : table->columns)
                {
                    // http://dev.mysql.com/doc/refman/5.1/en/create-table.html
                    /*
                    column_definition:
                        data_type [NOT NULL | NULL] [DEFAULT default_value]
                            [AUTO_INCREMENT] [UNIQUE [KEY] | [PRIMARY] KEY]
                            [COMMENT 'string']
                            [COLUMN_FORMAT {FIXED|DYNAMIC|DEFAULT}]
                            [STORAGE {DISK|MEMORY|DEFAULT}]
                            [reference_definition]

                    data_type:
                        BIT[(length)]
                        | TINYINT[(length)] [UNSIGNED] [ZEROFILL]
                        | SMALLINT[(length)] [UNSIGNED] [ZEROFILL]
                        | MEDIUMINT[(length)] [UNSIGNED] [ZEROFILL]
                        | INT[(length)] [UNSIGNED] [ZEROFILL]
                        | INTEGER[(length)] [UNSIGNED] [ZEROFILL]
                        | BIGINT[(length)] [UNSIGNED] [ZEROFILL]
                        | REAL[(length,decimals)] [UNSIGNED] [ZEROFILL]
                        | DOUBLE[(length,decimals)] [UNSIGNED] [ZEROFILL]
                        | FLOAT[(length,decimals)] [UNSIGNED] [ZEROFILL]
                        | DECIMAL[(length[,decimals])] [UNSIGNED] [ZEROFILL]
                        | NUMERIC[(length[,decimals])] [UNSIGNED] [ZEROFILL]
                        | DATE
                        | TIME
                        | TIMESTAMP
                        | DATETIME
                        | YEAR
                        | CHAR[(length)]
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | VARCHAR(length)
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | VARBINARY(length)
                        | BINARY[(length)]
                        | TINYBLOB
                        | BLOB
                        | MEDIUMBLOB
                        | LONGBLOB
                        | TINYTEXT [BINARY]
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | TEXT [BINARY]
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | MEDIUMTEXT [BINARY]
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | LONGTEXT [BINARY]
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | ENUM(value1,value2,value3,...)
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | SET(value1,value2,value3,...)
                            [CHARACTER SET charset_name] [COLLATE collation_name]
                        | spatial_type
                    */
                    logfile->Write(_T("    %s %s"), column->name, column->data_type ? column->data_type : this->default_data_type);

                    if (column->length)
                        logfile->Write(_T("(%d)"), column->length);

                    if (c++ + 1 < table->columns.size())
                        logfile->WriteLine(_T(","));
                    else
                        logfile->WriteLine(_T(""));
                }
                logfile->Write(_T(")"));
            }
            logfile->WriteLine(_T(";"));
            logfile->WriteLine(_T(""));
                
            if (table->columns.size())
            {
                RowColumnsPairMap rows = GetRowColumns(table);
                if (rows.size())
                {
                    logfile->Write(_T("INSERT INTO %s ("), table->name);
                    int c = 0;
                    for (auto column : table->columns)
                    {
                        if (c++)
                            logfile->Write(_T(", "));

                        logfile->Write(_T("%s"), column->name);
                    }
                    logfile->WriteLine(_T(") VALUES"));
                        
                    int row_count = 0;
                    for (auto row : rows)
                    {
                        if (row_count)
                            logfile->WriteLine(_T(","));

                        logfile->Write(_T("("));
                        std::vector<RowColumnPair> row_column = row.second;
                        c=  0;
                        for (auto column : table->columns)
                        {
                            if (c++)
                                logfile->Write(_T(", "));

                            auto search = std::find_if(row_column.begin(), row_column.end(), [column](RowColumnPair& pair) { return pair.first == column; });
                            if (search != row_column.end())
                            {
                                TCHAR *data = search->second->data;
                                data = SqlEscape(data);
                                if (data)
                                {
                                    logfile->Write(_T("'%s'"), data);
                                    delete[] data;
                                }
                                else
                                {
                                    logfile->Write(_T("''"));
                                }
                                    
                            }
                            else
                            {
                                logfile->Write(_T("NULL"));
                            }
                        }
                        logfile->Write(_T(")"));
                        row_count++;
                    }
                    logfile->WriteLine(_T(";"));
                }
            }
        }
        delete logfile;
    }
}

MysqlWriter::RowColumnsPairMap MysqlWriter::GetRowColumns(HistoryTable *table)
{
    RowColumnsPairMap rows;
    if (!table)
        return rows;
    
    for (auto column : table->columns)
    {
        for (auto row : column->rows)
        {
            RowColumnsPairMap::iterator rowMap = rows.find(row->id);
            if (rowMap == rows.end())
                rowMap = rows.insert({ row->id, std::vector<RowColumnPair>() }).first;
                
            RowColumnPair pair = RowColumnPair(column, row);
            std::vector<RowColumnPair> *vector = &rowMap->second;
            vector->insert(vector->end(), pair);
        }
    }
    return rows;
}

TCHAR *MysqlWriter::SqlEscape(TCHAR *query) const
{
    size_t len = _tcslen(query);
    if (!len)
        return 0;

    // Suppose we have to escape each character
    TCHAR *buffer = new TCHAR[len * 2 + 1], *p = buffer;
    memset(buffer, 0, (len * 2 + 1) * sizeof(TCHAR));

    TCHAR special_characters[] = { _T("\\'") };

    for (size_t i = 0; i < len; i++)
    {
        for (size_t l = 0; l < sizeof(special_characters) / sizeof(TCHAR); l++)
        {
            if (*query == special_characters[l])
            {
                *p++ = _T('\\');
                break;
            }
        }
        *p++ = *query++;
    }
    return buffer;
}
