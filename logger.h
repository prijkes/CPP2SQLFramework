#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include <string.h>
#include <windows.h>
#include <stdlib.h>

#include <fstream>
#include <list>
#include <algorithm>

#include "log.h"
#include "log_file.h"
#include "log_console.h"

#define LOG(level, ...) Logger::GetInstance().Log((level), ##__VA_ARGS__)

class Logger
{
private:
    // Dont forget to declare these two. You want to make sure they
    // are unaccessable otherwise you may accidently get copies o
    // your singleton appearing.
    // Don't Implement
    Logger(Logger const&);
    void operator=(Logger const&);

    std::list<Log*> handlers;
    size_t handle_count;
    
public:
    Logger();
    ~Logger();

    static Logger& GetInstance()
    {
        // Guaranteed to be destroyed.
        static Logger instance;

        // Instantiated on first use.
        return instance;
    }

    void AddLogger(::Log *log);
    void RemoveLogger(::Log *log);

    int SetDebug(int debug);
    int GetDebug();

    std::wofstream* GetFileHandle();

    void Log(char *format, ...);
    void Log(wchar_t *format, ...);
    void Log(int level, char *format, ...);
    void Log(int level, wchar_t *format, ...);

    void Log(bool add_newline, char *format, ...);
    void Log(bool add_newline, wchar_t *format, ...);
    void Log(bool add_newline, int level, char *format, ...);
    void Log(bool add_newline, int level, wchar_t *format, ...);

    void VLog(int level, char *format, va_list args);
    void VLog(char *format, va_list args);
    void VLog(int level, wchar_t *format, va_list args);
    void VLog(wchar_t *format, va_list args);

    void VLog(int level, char *format, va_list args, bool add_newline);
    void VLog(char *format, va_list args, bool add_newline);
    void VLog(int level, wchar_t *format, va_list args, bool add_newline);
    void VLog(wchar_t *format, va_list args, bool add_newline);
};

#endif
