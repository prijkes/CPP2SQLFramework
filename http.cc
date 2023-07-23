#include "Http.h"

Http::Http()
{
    if (WSAStartup(MAKEWORD(2, 2), &this->wsa))
        set_error(_T("failed to initialize WSA"));

    this->dynamic_start = 0;
    this->dynamic_end = 0;
    this->cookie = 0;
    this->interval = 300;   // Defaultl pause between each request, in milliseconds
}

Http::~Http()
{
    WSACleanup();
}

hostent *Http::ResolveHost(const char *host)
{
    // remoteHost is deleted by winSock itself - http://msdn.microsoft.com/en-us/library/ms738524(VS.85).aspx
    hostent *remoteHost = gethostbyname(host);
    return remoteHost;
}

TCHAR *Http::Send(TCHAR *host, unsigned short port, TCHAR *url, char *postdata)
{
    // Use WinHTTP to communicate
    HINTERNET hSession = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        set_error(_T("failed to open http session"));
        return 0;
    }

    // Create the WinHTTP connect handle
    HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        set_error(_T("failed to connect to http server"));
        return 0;
    }

    // Create the HTTP request handle
    const TCHAR *method = (postdata ? _T("POST") : _T("GET"));
    const TCHAR *version = _T("HTTP/1.1");
    const TCHAR *accept = _T("*/*");
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method, url, version, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        set_error(_T("failed to open http request"));
        return 0;
    }

    // Use any configured proxy if available
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG IeProxy;
    memset(&IeProxy, 0, sizeof(IeProxy));
    bool AutoDetectProxy = false;
    WINHTTP_AUTOPROXY_OPTIONS AutoProxyOptions;
    memset(&AutoProxyOptions, 0, sizeof(AutoProxyOptions));
    WINHTTP_PROXY_INFO ProxyInfo;
    memset(&ProxyInfo, 0, sizeof(ProxyInfo));
    DWORD cbProxyInfoSize = sizeof(ProxyInfo);

    if (WinHttpGetIEProxyConfigForCurrentUser(&IeProxy))
    {
        ProxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        ProxyInfo.lpszProxy = IeProxy.lpszProxy;
        ProxyInfo.lpszProxyBypass = IeProxy.lpszProxyBypass;

        AutoDetectProxy = IeProxy.fAutoDetect != 0;
        if (IeProxy.lpszAutoConfigUrl != NULL)
        {
            AutoDetectProxy = true;
            AutoProxyOptions.lpszAutoConfigUrl = IeProxy.lpszAutoConfigUrl;
        }
    }

    if (AutoDetectProxy)
    {
        if (AutoProxyOptions.lpszAutoConfigUrl != NULL)
        {
            AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
        }
        else
        {
            // Use auto-detection because the Proxy 
            // Auto-Config URL is not known.
            AutoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;

            // Use DHCP and DNS-based auto-detection.
            AutoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        }

        // If obtaining the PAC script requires NTLM/Negotiate
        // authentication, then automatically supply the client
        // domain credentials.
        AutoProxyOptions.fAutoLogonIfChallenged = TRUE;

        //
        // Call WinHttpGetProxyForUrl with our target URL. If 
        // auto-proxy succeeds, then set the proxy info on the 
        // request handle. If auto-proxy fails, ignore the error 
        // and attempt to send the HTTP request directly to the 
        // target server (using the default WINHTTP_ACCESS_TYPE_NO_PROXY 
        // configuration, which the requesthandle will inherit 
        // from the session).
        //
        memset(&ProxyInfo, 0, sizeof(ProxyInfo));
        if (!WinHttpGetProxyForUrl(hSession, url, &AutoProxyOptions, &ProxyInfo))
        {
            set_error(_T("failed to set automatic proxy options"));
        }
    }

    // A proxy configuration was found, set it on the request handle.
    if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &ProxyInfo, sizeof(ProxyInfo)))
    {
        // Exit if setting the proxy info failed.
        set_error(_T("ERROR setting proxy on the HTTP request"));
    }

    // Add header.
    size_t hdr_length = 1024 + (this->cookie ? _tcslen(this->cookie) : 0), index = 0;
    TCHAR *hdr = new TCHAR[hdr_length];
    memset(hdr, 0, hdr_length * sizeof(TCHAR));
    index +=_stprintf_s(hdr + index, hdr_length - index, _T("Connection: close"));
    if (cookie)
        index += _stprintf_s(hdr + index, hdr_length - index, _T("\r\nCookie: %s"), cookie);

    if (postdata)
        index += _stprintf_s(hdr + index, hdr_length - index, _T("\r\nContent-Type: application/x-www-form-urlencoded"));

    postdata = (postdata ? postdata : WINHTTP_NO_REQUEST_DATA);
    size_t postdata_length = (postdata ? strlen(postdata) * sizeof(char) : 0);
    BOOL success = WinHttpSendRequest(hRequest, hdr, _tcslen(hdr), postdata, postdata_length, postdata_length, 0);
    delete[] hdr;

    if (!success)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        set_error(_T("failed to send http request"));
        return 0;
    }

    if (!WinHttpReceiveResponse(hRequest, 0))
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        set_error(_T("no response from server"));
        return 0;
    }

    if (ProxyInfo.lpszProxy != NULL)
        GlobalFree(ProxyInfo.lpszProxy);

    if (ProxyInfo.lpszProxyBypass != NULL)
        GlobalFree(ProxyInfo.lpszProxyBypass);

    if (IeProxy.lpszAutoConfigUrl != NULL)
        GlobalFree(IeProxy.lpszAutoConfigUrl);

    TCHAR hdrBuf[1024] = { 0 }, *encoding = hdrBuf;
    unsigned long bufLen = 1024;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, hdrBuf, &bufLen, WINHTTP_NO_HEADER_INDEX))
    {
        encoding = _tcschr(hdrBuf, _T('='));
        if (encoding)
            encoding++;
    }

    //TCHAR *location = hdrBuf;
    //if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, hdrBuf, &bufLen, WINHTTP_NO_HEADER_INDEX))
    //{
    //    encoding = _tcschr(hdrBuf, _T('='));
    //    if (encoding)
    //        encoding++;
    //}

    bufLen = 0;
    unsigned long bRecv = 0, bRead = 0;
    char *buffer = 0;
    do
    {
        bRecv = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bRecv))
            set_error(_T("error in http query data available"));

        unsigned long newBufLen = bufLen + 1;
        char *old = new char[newBufLen];
        memset(old, 0, newBufLen * sizeof(char));
        memcpy(old, buffer, bufLen * sizeof(char));
        if (buffer)
        {
            delete[] buffer;
            buffer = 0;
        }

        newBufLen = bufLen + bRecv + 1;
        buffer = new char[newBufLen];
        memset(buffer, 0, newBufLen * sizeof(char));
        memcpy(buffer, old, bufLen * sizeof(char));
        delete[] old;
        old = 0;

        newBufLen = bRecv + 1;
        char* bufRecv = new char[newBufLen];
        memset(bufRecv, 0, newBufLen * sizeof(char));
        if (!WinHttpReadData(hRequest, bufRecv, bRecv, &bRead))
            set_error(_T("error in http read data"));

        memcpy(buffer + bufLen, bufRecv, bRead * sizeof(char));
        delete[] bufRecv;

        bufLen += bRead;
    } while (bRecv > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (!buffer || !bufLen)
    {
        delete[] buffer;
        return 0;
    }

    if (encoding && _tcsicmp(encoding, _T("none")) == 0)
        encoding = 0;
    
    if (!encoding)
    {
        if (this->GetEncodingFromMetaData(buffer, bufLen, hdrBuf, 1024))
            encoding = hdrBuf;
    }

    unsigned encodingIdentifier = encoding ? this->GetEncodingIdentifier(encoding) : 0;
    TCHAR *data = this->ConvertHttpData(buffer, bufLen, encodingIdentifier);
    delete[] buffer;
    return data;
}

unsigned int Http::GetEncodingIdentifier(TCHAR* encoding)
{
    for (unsigned int i = 0; i < sizeof(g_codepages) / sizeof(CodePages); i++)
    {
        if (!_tcsicmp(encoding, g_codepages[i].netname))
            return g_codepages[i].identifier;
    }
    return 0;
}

TCHAR *Http::ConvertHttpData(char *data, unsigned long len, unsigned int encodingIdentifier)
{
    TCHAR *utf16 = new TCHAR[len + 1];
    memset(utf16, 0, len * sizeof(TCHAR));
    if (!encodingIdentifier)
    {
        // Unknown encoding
        size_t count = 0;
        if (mbstowcs_s(&count, utf16, len + 1, data, len) || !count)
        {
            delete[] utf16;
            set_error(_T("failed at function mbstowcs_s in convert_http_data"));
            return 0;
        }
        return utf16;
    }

    //this->log(4, _T("[*] Converting received data from %s to UTF16"), codepages[i].netname);
    if (!MultiByteToWideChar(encodingIdentifier, 0, data, len, utf16, len + 1))
    {
        delete[] utf16;
        set_error(_T("failed at function MultiByteToWideChar in convert_http_data"));
        return 0;
    }
    return utf16;
}

bool Http::GetEncodingFromMetaData(char *page, unsigned long len, TCHAR *buffer, unsigned long buflen)
{
    if (!page || !len || !buffer || !buflen)
        return false;

    char *start = strstr(page, "<meta");
    if (!start)
        return false;

    char *end = strstr(start, "<title>");
    if (!end)
        return false;

    char *encoding = strstr(start, "charset=");
    if ((!encoding) && (encoding > end))
        return false;

    if (encoding)
        encoding += 8;

    len += 1;
    char *cbuf = new char[len];
    memset(cbuf, 0, len * sizeof(char));
    for (unsigned int i = 0; (encoding[i] != '"') && (encoding[i] != ' ') && (i < buflen); i++)
        cbuf[i] = encoding[i];

    size_t size = mbstowcs_s(&size, buffer, buflen, cbuf, strlen(cbuf));
    delete[] cbuf;
    return !size;
}

TCHAR *Http::GetData(TCHAR *host, unsigned short port, TCHAR *url)
{
    return PostData(host, port, url, 0);
}

TCHAR *Http::PostData(TCHAR *host, unsigned short port, TCHAR *url, char *postdata)
{
    LOG(4, _T("[*] SEND: %s"), url);
    if (postdata)
        LOG(4, _T("[*] %s"), postdata);

    TCHAR *data = Send(host, port, url, postdata);
    if (!data)
    {
        //LOG(0, _T("[-] Error: %s - %d"), getError(), getErrorCode());
        return 0;
    }

    unsigned long buf_len = _tcslen(data);

    if (this->dynamic_start && this->dynamic_end)
        this->RemoveDynamicContent(data, buf_len);

    LOG(4, _T("[*] RECV: %s"), data);
    this->requests++;
    Sleep(this->interval);

    return data;
}

void Http::RemoveDynamicContent(TCHAR *data, unsigned long data_len)
{
    TCHAR *startpos = _tcsstr(data, this->dynamic_start);                               // Points to the first char
    if (!startpos)
        return; // String not found

    TCHAR *endpos = _tcsstr(startpos, this->dynamic_end) + _tcslen(this->dynamic_end);  // Points to the char after the last char
    if (!endpos)
        return;    // Endpos not found

    unsigned startlen = data_len - _tcslen(startpos);
    unsigned long endlen = _tcslen(endpos);
    memmove(startpos, endpos, endlen * sizeof(TCHAR));
    memset(startpos + endlen, 0, (data_len - startlen - endlen) * sizeof(TCHAR));
}

void Http::set_error(TCHAR *error)
{
    this->error_ = error;
}

TCHAR *Http::ConvertCodePage(IN char *src, IN unsigned long src_len, int src_codepage, int dst_codepage)
{
    TCHAR *utf16 = 0;

    const CodePages *src_codepage_info = 0, *dst_codepage_info = 0;
    for (unsigned int i = 0; i < sizeof(g_codepages) / sizeof(CodePages); i++)
    {
        if (g_codepages[i].identifier == src_codepage)
            src_codepage_info = &g_codepages[i];

        if (g_codepages[i].identifier == dst_codepage)
            dst_codepage_info = &g_codepages[i];
    }

    if (!src_codepage_info && !dst_codepage_info)
        set_error(_T("source and destination codepage not found in codepages list"));
    else if (!src_codepage_info)
        set_error(_T("source codepage not found in codepages list"));
    else if (!dst_codepage_info)
        set_error(_T("destination codepage not found in codepages list"));
    else
    {
        //this->log(4, _T("[*] Converting received data from %s to UTF16"), codepages[i].netname);
        /*if (!MultiByteToWideChar(g_codepages[i].identifier, 0, data, len, utf16, len + 1))
        {
            delete[] utf16;
            set_error(_T("failed at function MultiByteToWideChar in convert_http_data"));
            return 0;
        }*/
        return utf16;
    }
    return utf16;
}
