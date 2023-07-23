#include "logger.h"

Logger::Logger()
{
    AddLogger(new LogConsole());
    AddLogger(new LogFile());
}

Logger::~Logger()
{
    for (auto handler : handlers)
        delete handler;
    
    handlers.clear();
}

void Logger::AddLogger(::Log *log)
{
    handlers.insert(handlers.end(), log);
}

void Logger::RemoveLogger(::Log *log)
{
    handlers.remove_if([log](::Log *i)
        {
            if (i == log) {
                delete i;
                return true;
            }
            return false;
        }
    );
}

std::wofstream* Logger::GetFileHandle()
{
    for (auto handler : handlers)
    {
        if (LogFile *file = dynamic_cast<LogFile*>(handler))
            return &file->file_handle();
    }
    return 0;
}

void Logger::Log(wchar_t *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(text, args);
    va_end(args);
}

void Logger::Log(int level, wchar_t *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(level, text, args);
    va_end(args);
}

void Logger::Log(bool add_newline, wchar_t *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(text, args, add_newline);
    va_end(args);
}

void Logger::Log(bool add_newline, int level, wchar_t *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(level, text, args, add_newline);
    va_end(args);
}

void Logger::Log(char *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(text, args);
    va_end(args);
}

void Logger::Log(int level, char *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(level, text, args);
    va_end(args);
}

void Logger::Log(bool add_newline, char *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(text, args);
    va_end(args);
}

void Logger::Log(bool add_newline, int level, char *text, ...)
{
    va_list args;
    va_start(args, text);
    this->VLog(level, text, args, add_newline);
    va_end(args);
}

void Logger::VLog(int level, char *format, va_list args)
{
    for (auto handler : handlers)
        handler->VWrite(level, format, args, true);
}
void Logger::VLog(char *format, va_list args)
{
    for (auto handler : handlers)
        handler->VWrite(format, args, true);    
}

void Logger::VLog(int level, wchar_t *format, va_list args)
{
    for (auto handler : handlers)
        handler->VWrite(level, format, args, true);
}

void Logger::VLog(wchar_t *format, va_list args)
{
    for (auto handler : handlers)
        handler->VWrite(format, args, true);
}

void Logger::VLog(int level, char *format, va_list args, bool add_newline)
{
    for (auto handler : handlers)
        handler->VWrite(level, format, args, add_newline);
}
void Logger::VLog(char *format, va_list args, bool add_newline)
{
    for (auto handler : handlers)
        handler->VWrite(format, args, add_newline);
}

void Logger::VLog(int level, wchar_t *format, va_list args, bool add_newline)
{
    for (auto handler : handlers)
        handler->VWrite(level, format, args, add_newline);
}

void Logger::VLog(wchar_t *format, va_list args, bool add_newline)
{
    for (auto handler : handlers)
        handler->VWrite(format, args, add_newline);
}

int Logger::SetDebug(int debug)
{
    for (auto handler : handlers)
        handler->debug = debug;
    
    return debug;
}

int Logger::GetDebug()
{
    for (auto handler : handlers)
        return handler->debug;

    return 0;
}
