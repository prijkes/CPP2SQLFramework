#ifndef _SQL_WRITER_H_
#define _SQL_WRITER_H_

#include "logger.h"
#include "history.h"


class SqlWriter
{
public:
    SqlWriter();
    ~SqlWriter();

    virtual void WriteFile(TCHAR *filename, HistoryDatabase *db);
    virtual void WriteCons(HistoryTable *table);
};

#endif
