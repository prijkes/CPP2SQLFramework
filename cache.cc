#include "Cache.h"

Cache::Cache(CachingType type)
{
    this->type = type;
    this->http = &Http::GetInstance();

    this->dynamic_start = 0;
    this->dynamic_end = 0;
    this->cache_list = 0;
    this->cache_pages_count = 0;
    this->cache_needles_count = 0;
    this->cache_start = 0;
    this->cache_max_pages = 0;
}

Cache::~Cache()
{
    if (this->dynamic_start)
        delete[] this->dynamic_start;

    if (this->dynamic_end)
        delete[] this->dynamic_end;
}

bool Cache::UpdateCacheList(TCHAR *host, unsigned short port, TCHAR *path, TCHAR *rest, unsigned long start_page, unsigned long end_page, TCHAR **keywords, int keywords_count)
{
    if (this->type == kNumberic)
    {
        this->cache_start = start_page;
        this->cache_max_pages = end_page - start_page;
    }
    else
    {
        this->cache_start = 0;
        this->cache_max_pages = keywords_count - 1;
    }
    unsigned long len = 0;
    unsigned long start = GetTickCount();
    unsigned long cpages = this->cache_pages_count;
    unsigned long max_not_found_pages = 20;
    unsigned long requests = 0;
    PCacheList *cache_tmp_list = 0;
    if (this->cache_list)
    {
        unsigned long pages = this->cache_max_pages - cpages;
        if (pages < 1)
        {
            LOG(1, _T("[*] Nothing extra to cache"));
            return true;
        }
        LOG(1, _T("[*] Caching %d extra page(s)"), pages);
        this->cache_max_pages = pages;

        /*
        set this->cache_list size to new size;
        create tmp storage and copy current cache_list to it
        delete old cache_list and reallocte with bigger size
        copy tmp storage to new cache_list and delete tmp
        */
        len = cpages + 1;
        PCacheList *tmp = new PCacheList[len];
        memset(tmp, 0, len * sizeof(PCacheList));
        memcpy(tmp, this->cache_list, cpages * sizeof(PCacheList));
        delete[] this->cache_list;

        len = cpages + pages + 1;
        this->cache_list = new PCacheList[len];
        memset(this->cache_list, 0, len * sizeof(PCacheList));
        memcpy(this->cache_list, tmp, cpages * sizeof(PCacheList));
        delete[] tmp;
        len = pages * max_not_found_pages + 1;
        cache_tmp_list = new PCacheList[len];
        memset(cache_tmp_list, 0, len * sizeof(PCacheList));
    }
    else
    {
        len = this->cache_max_pages * max_not_found_pages + 1;
        this->cache_list = new PCacheList[len];
        memset(this->cache_list, 0, len * sizeof(PCacheList));

        cache_tmp_list = new PCacheList[len];
        memset(cache_tmp_list, 0, len * sizeof(PCacheList));
    }
    unsigned long total_tmp_pages = 0;
    unsigned long total_tmp_needles = 0;
    unsigned long fpages = cpages;
    unsigned long not_found_pages = 0;
    unsigned long pagenum = this->cache_start;
    int keyword_index = 0;
    this->requests_ = 0;
    TCHAR *keyword = (this->type == kString) ? keywords[keyword_index] : 0;
    LOG(1, _T("[*] Creating cache list, please wait..."));

    while (true)
    {
        if (pagenum == this->cache_start + this->cache_max_pages)
        {
            LOG(1, _T("[*] Reached page %d - stopping with caching"), pagenum);
            break;
        }
        else if (not_found_pages == max_not_found_pages)
        {
            LOG(1, _T("[-] Found %d pages in a row without results, stopping"), not_found_pages);
            break;
        }

        for (unsigned long i = 0; i < total_tmp_pages; i++)
        {
            if (cache_tmp_list[i]->page == pagenum)
            {
                i = 0;
                pagenum++;
            }
        }

        len = (_tcslen(path) + (this->type == kString ? _tcslen(keyword) : 0) + _tcslen(rest) + 1) * sizeof(TCHAR) + 1024;
        TCHAR *qry = new TCHAR[len];
        memset(qry, 0, len * sizeof(TCHAR));
        if (this->type == kNumberic)
        {
            _stprintf_s(qry, len, _T("%s%d%s"), path, pagenum, rest);
        }
        else
        {
            _tcscat_s(qry, len, path);

            // If we can't use quotes (ie. the last char isn't the opening quote), use MySQL functions
            if (path[_tcslen(path) - 1] != _T('\''))
            {
                _tcscat_s(qry, len, _T("CONCAT("));
                for (unsigned int z = 0; z < _tcslen(keyword); z++)
                {
                    if (z)
                        _tcscat_s(qry, len, _T(","));

                    _stprintf_s(qry, len, _T("%sCHAR(%d)"), qry, keyword[z]);
                }
                _tcscat_s(qry, len, _T(")"));
            }
            else
            {
                // Otherwise just paste the keyword as-is
                _tcscat_s(qry, len, keyword);
            }
            _tcscat_s(qry, len, rest);
        }

        TCHAR *data = this->SendRequest(host, port, qry);
        delete[] qry;
        if (!data)
            return false;

        requests++;
        cache_tmp_list[total_tmp_pages] = new CacheList;
        PCacheList curpagecache = cache_tmp_list[total_tmp_pages];
        memset(curpagecache, 0, sizeof(CacheList));

        len = _tcslen(data) + 1;
        curpagecache->data = new TCHAR[len];
        memset(curpagecache->data, 0, len * sizeof(TCHAR));
        _tcscpy_s(curpagecache->data, len, data);
        delete[] data;

        data = curpagecache->data;
        curpagecache->page = pagenum;
        if (this->type == kString)
            _tcscpy_s(curpagecache->keyword, keyword);

        curpagecache->index = total_tmp_pages;
        curpagecache->data_needles = new PCacheNeedle[len];
        curpagecache->real_needles = new PCacheNeedle[len];
        memset(curpagecache->data_needles, 0, len * sizeof(PCacheNeedle));
        memset(curpagecache->real_needles, 0, len * sizeof(PCacheNeedle));
        curpagecache->data_needles_count = this->FillNeedleList(data, len - 1, curpagecache->data_needles);
        curpagecache->real_needles_count = 0;
        unsigned int nindex = 0;
        PCacheNeedle *strings = curpagecache->data_needles;
        if (this->type == kNumberic)
            LOG(2, _T("[*] cached page[%d] in cache_index[%d]"), pagenum, total_tmp_pages);
        else
            LOG(2, _T("[*] cached keyword[%s] in cache_index[%d]"), keyword, total_tmp_pages);

        for (unsigned int a = 0; a < curpagecache->data_needles_count; a++)
        {
            TCHAR *needle = strings[a]->needle;
            if (this->type == kNumberic)
                LOG(3, _T("[*]\ttrying to find page[%d]index[%d]needle[%s] in cached pages"), pagenum, a, needle);
            else
                LOG(3, _T("[*]\ttrying to find keyword[%s]index[%d]needle[%s] in cached pages"), keyword, a, needle);

            bool unique = true;
            for (unsigned int b = 0; b < total_tmp_pages; b++)
            {
                PCacheList cached = cache_tmp_list[b];
                if (cached->page == curpagecache->page)
                    continue;

                TCHAR *data2 = cached->data;
                PCacheNeedle *strings2 = cached->data_needles;
                if (this->type == kNumberic)
                    LOG(4, _T("\t\ttrying cached_page[%d]cached_index[%d]needles[%d]cached_data[%s]"), cached->page, b, cached->data_needles_count, data2);
                else
                    LOG(4, _T("\t\ttrying cached_keyword[%s]cached_index[%d]needles[%d]cached_data[%s]"), cached->keyword, b, cached->data_needles_count, data2);

                for (unsigned int c = 0; c < cached->data_needles_count; c++)
                {
                    TCHAR *needle2 = strings2[c]->needle;
                    if (this->type == kNumberic)
                        LOG(4, _T("\t\t\tcached_page[%d]cached_index[%d]needle_index[%d]needle[%s]"), cached->page, b, c, needle2);
                    else
                        LOG(4, _T("\t\t\tcached_keyword[%s]cached_index[%d]needle_index[%d]needle[%s]"), cached->keyword, b, c, needle2);

                    if (!_tcscmp(needle2, needle))
                    {
                        /*
                        We found the same data_needle in a different cached page.
                        */
                        unique = false;
                        for (unsigned int d = 0; d < cached->real_needles_count; d++)
                        {
                            /*
                            Check if the needle has been added to the unique needle list of the same cached page.
                            If so, remove it and re-index the array list of unique needles.
                            */
                            if (!_tcscmp(needle2, cached->real_needles[d]->needle))
                            {
                                delete[] cached->real_needles[d]->needle;
                                delete cached->real_needles[d];
                                for (; d + 1 < cached->real_needles_count; d++)
                                    cached->real_needles[d] = cached->real_needles[d + 1];

                                cached->real_needles_count--;
                                if (!cached->real_needles_count)
                                {
                                    fpages--;
                                    if (this->type == kNumberic)
                                        LOG(2, _T("all needles deleted from cached_page[%d]cached_index[%d]"), cached->page, cached->index);
                                    else
                                        LOG(2, _T("all needles deleted from cached_keyword[%s]cached_index[%d]"), cached->keyword, cached->index);
                                }
                                total_tmp_needles--;
                                break;
                            }
                        }
                        LOG(4, _T("\t\t\t\tfound same needle[%s] and needle2[%s] -- removed"), needle, needle2);
                    }		// If the needles are the same

                    if (!unique)
                        break;
                }			// Cached page data needles count

                if (!unique)
                    break;
            }				// Cached tmp pages count

            if (unique)
            {
                /*
                Unique needle found, check if we already have the same needle in our cache list.
                If we don't, add it, else skip it.
                */
                bool already_in_cache = false;
                for (unsigned int i = 0; i < curpagecache->real_needles_count; i++)
                {
                    if (!_tcscmp(curpagecache->real_needles[i]->needle, needle))
                    {
                        already_in_cache = true;
                        break;
                    }
                }

                if (!already_in_cache)
                {
                    if (this->type == kNumberic)
                        LOG(3, _T("found unique needle_index[%d]needle[%s] for page[%d]cache_index[%d]"), nindex, needle, curpagecache->page, total_tmp_pages);
                    else
                        LOG(3, _T("found unique needle_index[%d]needle[%s] for keyword[%s]cache_index[%d]"), nindex, needle, curpagecache->keyword, total_tmp_pages);

                    curpagecache->real_needles[nindex] = new CacheNeedle;
                    len = _tcslen(needle) + 1;
                    curpagecache->real_needles[nindex]->needle = new TCHAR[len];
                    memset(curpagecache->real_needles[nindex]->needle, 0, len * sizeof(TCHAR));
                    _tcscpy_s(curpagecache->real_needles[nindex++]->needle, len, needle);
                    curpagecache->real_needles_count++;
                    total_tmp_needles++;
                }			// If we have the needle not in cache
            }				// If the needle is unique
        }					// Current page data needles count

        if (nindex)
        {
            /*
            We have found atleast one needle that didn't appear in any other cached pages.
            */
            if (this->type == kNumberic)
                LOG(2, _T("found needles[%d] for page[%d]cache_index[%d] of total length[%d/%d]"), curpagecache->real_needles_count, pagenum, total_tmp_pages, ++fpages, this->cache_max_pages + 1);
            else
                LOG(2, _T("found needles[%d] for keyword[%s]cache_index[%d] of total length[%d/%d]"), curpagecache->real_needles_count, keyword, total_tmp_pages, ++fpages, this->cache_max_pages + 1);

            if (not_found_pages > 0)
                not_found_pages--;
            else
                not_found_pages = 0;
        }
        else
        {
            /*
            No unique needle found. Increase the counter for not found pages in a row.
            */
            if (this->type == kNumberic)
                LOG(2, _T("no needles found for page[%d]cache_index[%d]"), curpagecache->page, total_tmp_pages);
            else
                LOG(2, _T("no needles found for keyword[%s]cache_index[%d]"), curpagecache->keyword, total_tmp_pages);

            not_found_pages++;
        }					// If we found a unique needle
        total_tmp_pages++;

        if (this->type == kString)
            keyword = keywords[++keyword_index];
    }						// While we not have enough pages with unique needles in cache yet

    /*
    Fill real cache list.
    */
    for (unsigned long pi = 0; pi < total_tmp_pages; pi++)
    {
        PCacheList page = cache_tmp_list[pi];
        if (page->real_needles_count)
        {
            unsigned long index = this->cache_pages_count;
            this->cache_list[index] = new CacheList;
            PCacheList cptr = this->cache_list[index];
            memset(cptr, 0, sizeof(CacheList));
            cptr->page = page->page;
            _tcscpy_s(cptr->keyword, page->keyword);
            cptr->real_needles_count = page->real_needles_count;
            len = page->real_needles_count + 1;
            cptr->real_needles = new PCacheNeedle[len];
            memset(cptr->real_needles, 0, len*sizeof(PCacheNeedle));
            for (unsigned int i = 0; i < page->real_needles_count; i++)
            {
                TCHAR *needle = page->real_needles[i]->needle;
                cptr->real_needles[i] = new CacheNeedle;
                memset(cptr->real_needles[i], 0, sizeof(CacheNeedle));
                len = _tcslen(needle) + 1;
                cptr->real_needles[i]->needle = new TCHAR[len];
                memset(cptr->real_needles[i]->needle, 0, len * sizeof(TCHAR));
                _tcscpy_s(cptr->real_needles[i]->needle, len, needle);
                this->cache_needles_count++;
            }
            this->cache_pages_count++;
        }
    }

    /*
    Free/delete memory used for cache_tmp_list
    */
    for (unsigned int a = 0; a < total_tmp_pages; a++)
    {
        for (unsigned int b = 0; b < cache_tmp_list[a]->data_needles_count; b++)
        {
            delete[] cache_tmp_list[a]->data_needles[b]->needle;
            delete cache_tmp_list[a]->data_needles[b];
        }

        for (unsigned int b = 0; b < cache_tmp_list[a]->real_needles_count; b++)
        {
            delete[] cache_tmp_list[a]->real_needles[b]->needle;
            delete cache_tmp_list[a]->real_needles[b];
        }
        delete[] cache_tmp_list[a]->data;
        delete cache_tmp_list[a];
    }
    delete[] cache_tmp_list;
    unsigned long end = GetTickCount();
    unsigned long length = this->cache_pages_count;
    unsigned long total = this->cache_needles_count;
    if (length)
    {
        //LOG(2, _T("[*] Cache overview:"));
        //this->show_cache_list();
    }
    LOG(1, _T("[*] Total pages requested: %d"), total_tmp_pages);
    LOG(1, _T("[*] Total amount of needles found: %d"), total_tmp_needles);
    LOG(1, _T("[*] Amount of pages in cache: %d"), length);
    LOG(1, _T("[*] Amount of needles in cache: %d"), total);
    LOG(1, _T("[*] Average needles per page: %d"), (length ? total / length : 0));
    LOG(0, _T("[*] Generated %d request(s)"), requests);
    LOG(0, _T("[*] Caching done, took %d second(s)"), (end - start) / 1000);
    return true;
}

unsigned int Cache::FillNeedleList(TCHAR *data, unsigned int len, PCacheNeedle *needles)
{
    unsigned int index = 0, stringlen = 0;
    len += 1;
    TCHAR *tmp = new TCHAR[len];
    memset(tmp, 0, len * sizeof(TCHAR));
    memcpy(tmp, data, len * sizeof(TCHAR));
    TCHAR *context = 0;
    TCHAR delims[] = _T(" \t\r\n");
    TCHAR *token = _tcstok_s(tmp, delims, &context);
    while (token)
    {
        needles[index] = new CacheNeedle;
        memset(needles[index], 0, sizeof(CacheNeedle));
        stringlen = _tcslen(token) + 1;
        needles[index]->length = stringlen - 1;
        needles[index]->needle = new TCHAR[stringlen];
        memset(needles[index]->needle, 0, stringlen * sizeof(TCHAR));
        _tcscpy_s(needles[index]->needle, stringlen, token);
        index++;
        token = _tcstok_s(0, delims, &context);
    }
    delete[] tmp;
    return index;
}

long Cache::SearchCacheList(TCHAR *page)
{
    PCacheHit *cachehits = new PCacheHit[this->cache_pages_count + 1];
    memset(cachehits, 0, (this->cache_pages_count + 1) * sizeof(PCacheHit*));

    int total_hits = -1;
    unsigned int ncount = _tcslen(page) + 1;
    PCacheNeedle *tpage = new PCacheNeedle[ncount];
    memset(tpage, 0, ncount * sizeof(PCacheNeedle));
    ncount = this->FillNeedleList(page, ncount - 1, tpage);
    LOG(3, _T("[*] Target page: %s"), page);
    for (unsigned int a = 0; a < ncount; a++)
    {
        PCacheNeedle tneedle = tpage[a];
        LOG(3, _T("[*] Looking for needle[%s] in cache list"), tneedle->needle);
        for (unsigned int i = 0; i < this->cache_pages_count; i++)
        {
            unsigned long crpage = this->cache_list[i]->page;
            TCHAR *crkeyword = this->cache_list[i]->keyword;
            if (!cachehits[i])
                cachehits[i] = new CacheHit;

            for (unsigned int x = 0; x < this->cache_list[i]->real_needles_count; x++)
            {
                PCacheNeedle needle = this->cache_list[i]->real_needles[x];
                if (this->type == kNumberic)
                    LOG(4, _T("[*] Comparing cached[%d]page[%d]index[%d]needle[%s]"), i, crpage, x, needle->needle);
                else
                    LOG(4, _T("[*] Comparing cached[%d]keyword[%s]index[%d]needle[%s]"), i, crkeyword, x, needle->needle);

                if (!_tcscmp(needle->needle, tneedle->needle))
                {
                    if (this->type == kNumberic)
                        LOG(3, _T("[*] Found index[%d]needle[%s] in cached[%d]page[%d]index[%d]needle[%s]"), a, tneedle->needle, i, crpage, x, needle->needle);
                    else
                        LOG(3, _T("[*] Found index[%d]needle[%s] in cached[%d]keyword[%d]index[%d]needle[%s]"), a, tneedle->needle, i, crkeyword, x, needle->needle);

                    cachehits[i]->needlesfound++;
                    total_hits++;
                }
                cachehits[i]->chr = this->cache_list[i]->chr;
            }
        }
    }

    for (unsigned int i = 0; i < ncount; i++)
    {
        delete[] tpage[i]->needle;
        delete tpage[i];
    }
    delete[] tpage;

    if (total_hits == -1)
    {
        //LOG(4, _T("[-] Result not found in cache, try other method"));
        return -1;
    }

    /*
    Check for amount of hits we have in the cache list for current page.
    The cached page that has the most needle hits found is likely the correct page.
    */
    long chr = -1;
    unsigned long mostNeedles = 0;
    for (int i = 0; cachehits[i]; i++)
    {
        LOG(3, _T("[*] Found %d matching needles for cache index %d and char %c"), cachehits[i]->needlesfound, i, cachehits[i]->chr);
        if (cachehits[i]->needlesfound > mostNeedles)
        {
            mostNeedles = cachehits[i]->needlesfound;
            chr = cachehits[i]->chr;
        }
    }

    for (int i = 0; cachehits[i]; i++)
        delete cachehits[i];

    delete[] cachehits;
    return chr;
}

void Cache::Reset()
{
    // Cache list
    if (this->cache_list)
    {
        for (unsigned int i = 0; i < this->cache_pages_count; i++)
        {
            for (unsigned int x = 0; x < this->cache_list[i]->data_needles_count; x++)
            {
                delete[] this->cache_list[i]->data_needles[x]->needle;
                delete this->cache_list[i]->data_needles[x];
            }

            for (unsigned int y = 0; y < this->cache_list[i]->real_needles_count; y++)
            {
                delete[] this->cache_list[i]->real_needles[y]->needle;
                delete this->cache_list[i]->real_needles[y];
            }
            delete this->cache_list[i];
        }
        delete[] this->cache_list;
        this->cache_list = 0;
        this->cache_start = 0;
        this->cache_max_pages = 0;
        this->cache_pages_count = 0;
        this->cache_needles_count = 0;
        LOG(0, _T("[*] Cache cleared."));
    }
}

TCHAR *Cache::SendRequest(TCHAR *host, unsigned short port, TCHAR *qry)
{
    TCHAR *data = this->http->GetData(host, port, qry);
    this->requests_++;
    return data;
}
