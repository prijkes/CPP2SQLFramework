#ifndef _CACHE_H_
#define _CACHE_H_

#include <tchar.h>
#include <string.h>
#include <math.h>

#include "http.h"
#include "log.h"

#ifdef RESULT_NOT_FOUND
#undef RESULT_NOT_FOUND
#endif
#define RESULT_NOT_FOUND -1

typedef struct CacheNeedle
{
    CacheNeedle()
    {
        this->length = 0;
        this->needle = 0;
    }
    unsigned int length;   // length of needle string
    TCHAR *needle;         // needle string
} CacheNeedle, *PCacheNeedle;

typedef struct CacheList
{
    CacheList()
    {
        this->page = 0;
        this->data = 0;
        this->index = 0;
        this->data_needles = 0;
        this->data_needles_count = 0;
        this->real_needles = 0;
        this->real_needles_count = 0;
        this->chr = 0;
    }
    unsigned long page;                 // real page number
    TCHAR keyword[256];                 // keyword if pages aren't numberic but a string
    TCHAR *data;                        // page data
    unsigned long index;                // index id in cache_list array
    PCacheNeedle *data_needles;             // array of data split by '\r\n '
    unsigned long data_needles_count;   // array length
    PCacheNeedle *real_needles;             // array of unique needles
    unsigned long real_needles_count;   // array length
    long chr = 0;                      // chr used for this page
} CacheList, *PCacheList;

typedef struct CacheHit
{
    CacheHit()
    {
        this->needlesfound = 0;
        this->chr = 0;
    }
    unsigned long needlesfound;
    long chr;
} CacheHit, *PCacheHit;

enum CachingType
{
    kNumberic,
    kString
};

class Cache
{
private:
    Cache() {};

    Http *http;

    CachingType type;

    TCHAR *dynamic_start;
    TCHAR *dynamic_end;

    // caching method
    PCacheList *cache_list;                 // Final cache list
    unsigned long cache_pages_count;        // Amount of pages in cache
    unsigned long cache_needles_count;      // Amount of needles in cache

    unsigned long cache_start;              // Start page
    unsigned long cache_max_pages;          // Max pages to cache

    unsigned long requests_;

    TCHAR *SendRequest(TCHAR *host, unsigned short port, TCHAR *qry);

public:
    Cache(CachingType type);
    ~Cache();

    // Caching functions
    bool UpdateCacheList(TCHAR *host, unsigned short port, TCHAR *path, TCHAR *rest, unsigned long start_page, unsigned long end_page = 0, TCHAR **keywords = 0, int keywords_count = 0);                   // Create/updates the cache list
    long SearchCacheList(TCHAR *page);       // Search in cache list
    unsigned int FillNeedleList(TCHAR *data, unsigned int len, PCacheNeedle *needles);
    void Reset();                               // Reset cache list

    PCacheList *GetCacheList(unsigned long *cache_pages_count) const { *cache_pages_count = this->cache_pages_count; return this->cache_list; }

    TCHAR *HttpSend(TCHAR *url);
    void RemoveDynamicContent(TCHAR *page, unsigned long len);

    unsigned long requests() const { return this->requests_; }
};

#endif
