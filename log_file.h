#ifndef _LOG_FILE_H_
#define _LOG_FILE_H_

#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include <string.h>
#include <windows.h>

#include <stdlib.h>
#include <fstream>

#include "log.h"

class LogFile final : public Log
{
private:
    TCHAR *output_filename;             // output filename
    std::wofstream file_handle_;       // handle to output filename

    // Dont forget to declare these two. You want to make sure they
    // are unaccessable otherwise you may accidently get copies of
    // your singleton appearing.
    // Don't Implement
    LogFile(LogFile const&);
    void operator=(LogFile const&);

public:
    LogFile(TCHAR *filename = 0);
    LogFile(TCHAR *filename, bool write_utf8bom);
    ~LogFile();

    static LogFile& GetInstance()
    {
        // Guaranteed to be destroyed.
        static LogFile instance;

        // Instantiated on first use.
        return instance;
    }

    std::wofstream& file_handle() { return this->file_handle_; }

    void VWrite(char *format, va_list args, bool add_newline) override;
    void VWrite(wchar_t *format, va_list args, bool add_newline) override;
};

#endif
