#ifndef _LOG_CONSOLE_H_
#define _LOG_CONSOLE_H_

#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include <string.h>
#include <windows.h>

#include <stdlib.h>
#include <fstream>

#include "log.h"

class LogConsole final : public Log
{
private:
    // Dont forget to declare these two. You want to make sure they
    // are unaccessable otherwise you may accidently get copies o
    // your singleton appearing.
    // Don't Implement
    LogConsole(LogConsole const&);
    void operator=(LogConsole const&);

public:
    LogConsole();
    LogConsole(int debug);
    ~LogConsole();

    static LogConsole& GetInstance()
    {
        // Guaranteed to be destroyed.
        static LogConsole instance;

        // Instantiated on first use.
        return instance;
    }

    void VWrite(char *format, va_list args, bool add_newline) override;
    void VWrite(wchar_t *format, va_list args, bool add_newline) override;
};

#endif
