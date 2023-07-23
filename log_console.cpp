#include "log_console.h"

LogConsole::LogConsole()
{
}

LogConsole::LogConsole(int debug)
{
    this->debug = debug;
}

LogConsole::~LogConsole()
{
}

void LogConsole::VWrite(wchar_t *text, va_list args, bool add_newline)
{
    int size = _vscwprintf(text, args) + 2 + 1;	// \r\n + \0
    wchar_t *buffer = new wchar_t[size];
    memset(buffer, 0, size * sizeof(wchar_t));
    vswprintf_s(buffer, size, text, args);

    if (add_newline)
    {
        wchar_t *end = buffer + wcslen(buffer) - 2;
        if (wcscmp(end, L"\r\n"))
            wcscat_s(buffer, size, L"\r\n");
    }

    unsigned long bWritten = 0, maxBytesToWriteOnce = 10000, totalbBytesWritten = 0;
    unsigned long buflen = wcslen(buffer);
    do
    {
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), buffer + totalbBytesWritten, min(buflen - totalbBytesWritten, maxBytesToWriteOnce), &bWritten, 0);
        totalbBytesWritten += bWritten;
    } while (GetLastError() == 0 && bWritten > 0 && totalbBytesWritten < buflen);

    delete[] buffer;
}

void LogConsole::VWrite(char *text, va_list args, bool add_newline)
{
    int size = _vscprintf(text, args) + 2 + 1;		// \r\n + \0
    char *buffer = new char[size];
    memset(buffer, 0, size * sizeof(char));
    vsprintf_s(buffer, size, text, args);

    if (add_newline)
    {
        char *end = buffer + strlen(buffer) - 2;
        if (strcmp(end, "\r\n"))
            strcat_s(buffer, size, "\r\n");
    }

#if defined(UNICODE) || defined(_UNICODE)
    size_t count = 0, len = strlen(buffer) + 1;
    wchar_t *wcsbuf = new wchar_t[len];
    memset(wcsbuf, 0, len * sizeof(wchar_t));
    mbstowcs_s(&count, wcsbuf, len, buffer, len - 1);

    unsigned long bwritten;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), wcsbuf, wcslen(wcsbuf), &bwritten, 0);

    delete[] wcsbuf;
#else
    size_t btowrite = strlen(buffer);
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), buffer, btowrite, &bwritten, 0);
#endif
    delete[] buffer;
}
