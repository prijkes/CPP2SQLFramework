#include "sql_writer.h"

SqlWriter::SqlWriter()
{
}

SqlWriter::~SqlWriter()
{
}

void SqlWriter::WriteFile(TCHAR *filename, HistoryDatabase *db)
{
}

void SqlWriter::WriteCons(HistoryTable *table)
{
    if (!table)
        return;

    LOG(0, _T("\r\n"));
    LOG(0, _T("\t\t\t[+] Table %d: %s"), table->id, table->name);
    LOG(0, _T("\t\t\t[+] Columns: %d"), table->columncount);
    if (table->rowcount != -1)
        LOG(0, _T("\t\t\t[+] Rows: %d"), table->rowcount);

    for (auto column : table->columns)
    {
        LOG(0, _T("\r\n"));
        LOG(0, _T("\t\t\t\t[+] Column %d: %s"), column->id, column->name);
        for (auto row : column->rows)
        {
            LOG(0, _T("\r\n"));
            LOG(0, _T("\t\t\t\t\t[+] Row: %d"), row->id);
            LOG(0, _T("\t\t\t\t\t[+] Data Length: %d"), row->length);
            LOG(0, _T("\t\t\t\t\t[+] Data: %s"), row->data);
        }
    }
}
