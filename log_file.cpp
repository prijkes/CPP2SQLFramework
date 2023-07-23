#include "log_file.h"

LogFile::LogFile(TCHAR *filename) : LogFile(filename, true)
{
}

LogFile::LogFile(TCHAR *filename, bool write_utf8bom)
{
    if (!filename)
    {
        SYSTEMTIME time;
        GetSystemTime(&time);
        TCHAR datefilename[256] = { 0 };
        _stprintf_s(datefilename, 100, _T("SQL_%d%02d%02d_%02d%02d.log"), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
        filename = datefilename;
    }

    unsigned long length = _tcslen(filename) + 1;
    this->output_filename = new TCHAR[length];
    memset(this->output_filename, 0, length * sizeof(TCHAR));
    _tcscpy_s(this->output_filename, length, filename);

    this->file_handle_.open(this->output_filename, std::ios::out | std::ios::binary);

    if (write_utf8bom)
    {
        wchar_t utf8_bom[] = { 0xEF, 0xBB, 0xBF };
        this->file_handle_.write(utf8_bom, sizeof(utf8_bom) / sizeof(wchar_t));
    }
}

LogFile::~LogFile()
{
    if (this->output_filename)
        delete[] this->output_filename;

    if (this->file_handle_)
        this->file_handle_.close();
}

void LogFile::VWrite(wchar_t *text, va_list args, bool add_newline)
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

    if (this->file_handle_.is_open())
    {
        // Convert to UTF8
        int chars_needed = WideCharToMultiByte(CP_UTF8, 0, buffer, size, NULL, 0, NULL, NULL);

        if (chars_needed > 0)
        {
            char *new_buffer = new char[chars_needed];
            memset(new_buffer, 0, chars_needed * sizeof(char));
            WideCharToMultiByte(CP_UTF8, 0, buffer, size, new_buffer, chars_needed, NULL, NULL);
            this->file_handle_ << new_buffer;
            delete[] new_buffer;
        }
    }
    delete[] buffer;
}

void LogFile::VWrite(char *text, va_list args, bool add_newline)
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

    if (this->file_handle_.is_open())
    {
        // Convert to UTF8
        int chars_needed = WideCharToMultiByte(CP_UTF8, 0, wcsbuf, size, NULL, 0, NULL, NULL);

        if (chars_needed > 0)
        {
            char *new_buffer = new char[chars_needed];
            memset(new_buffer, 0, chars_needed * sizeof(char));
            WideCharToMultiByte(CP_UTF8, 0, wcsbuf, size, new_buffer, chars_needed, NULL, NULL);
            this->file_handle_ << new_buffer;
            delete[] new_buffer;
        }
    }
    delete[] wcsbuf;
#else
    size_t btowrite = strlen(buffer);
    if (this->file_handle_)
    {
        // Convert to UTF8
        int chars_needed = WideCharToMultiByte(CP_UTF8, 0, buffer, btowrite, NULL, 0, NULL, NULL);

        if (chars_needed > 0)
        {
            char *new_buffer = new char[chars_needed];
            WideCharToMultiByte(CP_UTF8, 0, buffer, btowrite, new_buffer, chars_needed, NULL, NULL);
            this->file_handle_ << new_buffer;
            delete[] new_buffer;
        }
    }
#endif
    delete[] buffer;

}
