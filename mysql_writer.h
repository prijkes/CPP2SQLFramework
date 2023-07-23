#ifndef _MYSQL_WRITER_H_
#define _MYSQL_WRITER_H_

#include "logger.h"
#include "history.h"
#include "sql_writer.h"

#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>

class MysqlWriter final : SqlWriter
{
private:
    typedef std::pair<HistoryColumn *, HistoryRow *> RowColumnPair;
    typedef std::map<int, std::vector<RowColumnPair>> RowColumnsPairMap;

    //
    // Functions
    //
    TCHAR *SqlEscape(TCHAR *query) const;
    RowColumnsPairMap GetRowColumns(HistoryTable *table);

    const TCHAR *default_data_type = _T("text");

public:
    MysqlWriter();
    ~MysqlWriter();

    void WriteFile(TCHAR *filename, HistoryDatabase *db) override;
    void WriteCons(HistoryTable *table);
};

#endif
