#include "log.h"

Log::Log()
{
    this->debug = 0;
}

Log::~Log()
{
}

void Log::VWrite(int level, wchar_t *text, va_list args, bool add_newline)
{
    if (this->debug >= level)
        this->VWrite(text, args, add_newline);
}

void Log::VWrite(wchar_t *text, va_list args, bool add_newline)
{
}

void Log::WriteLine(wchar_t *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VWrite(text, args, true);
    va_end(args);
}

void Log::WriteLine(int level, wchar_t *text, ...)
{
    if (this->debug >= level)
    {   
        va_list args;
        va_start(args, text);
        this->VWrite(text, args, true);
        va_end(args);
    }
}

void Log::Write(wchar_t *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VWrite(text, args, false);
    va_end(args);
}

void Log::Write(int level, wchar_t *text, ...)
{
    if (this->debug >= level)
    {
        va_list args;
        va_start(args, text);
        this->VWrite(text, args, false);
        va_end(args);
    }
}

void Log::VWrite(int level, char *text, va_list args, bool add_newline)
{
    if (this->debug >= level)
        this->VWrite(text, args, add_newline);
}

void Log::VWrite(char *text, va_list args, bool add_newline)
{
}

void Log::WriteLine(char *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VWrite(text, args, true);
    va_end(args);
}

void Log::WriteLine(int level, char *text, ...)
{
    if (this->debug >= level)
    {
        va_list args;
        va_start(args, text);
        this->VWrite(text, args, true);
        va_end(args);
    }
}

void Log::Write(char *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VWrite(text, args, false);
    va_end(args);
}

void Log::Write(int level, char *text, ...)
{
    if (this->debug >= level)
    {
        va_list args;
        va_start(args, text);
        this->VWrite(text, args, false);
        va_end(args);
    }
}
