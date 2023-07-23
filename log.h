#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include <string.h>
#include <windows.h>

#include <stdlib.h>
#include <fstream>

class Log
{
    public:
        Log();
        ~Log();

        int debug;                // Level of debug

        virtual void VWrite(int level, char *format, va_list args, bool add_newline);
        virtual void VWrite(char *format, va_list args, bool add_newline);
        virtual void VWrite(int level, wchar_t *format, va_list args, bool add_newline);
        virtual void VWrite(wchar_t *format, va_list args, bool add_newline);

        virtual void WriteLine(char *format, ...);
        virtual void WriteLine(wchar_t *format, ...);

        virtual void WriteLine(int level, char *format, ...);
        virtual void WriteLine(int level, wchar_t *format, ...);

        virtual void Write(char *format, ...);
        virtual void Write(wchar_t *format, ...);

        virtual void Write(int level, char *format, ...);
        virtual void Write(int level, wchar_t *format, ...);
};

#endif
