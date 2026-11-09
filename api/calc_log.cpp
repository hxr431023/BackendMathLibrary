// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_log.cpp — log.dll binding: lazy LoadLibrary from the module's own directory
// plus GraalVM isolate + log/cache calls.
// All calls are no-ops if log.dll is not found, so the DLL still works standalone.

#include <windows.h>
#include <string>
#include <cstring>
#include <mutex>

// Directory that holds CalcManager.dll — resolved by calc_native.cpp.
extern "C" const char* calc_module_dir();

namespace {
    HMODULE g_logDll = nullptr;
    void*   g_thread = nullptr;
    std::mutex g_initMutex;
    bool    g_triedInit = false;
    // Logging is off until the host turns it on (settings: "log_enabled").
    bool    g_enabled = false;

    typedef int  (*pfn_create_isolate)(void*, void**, void**);
    typedef int  (*pfn_detach)(void*);
    typedef int  (*pfn_log_init)(void*, const char*);
    typedef void (*pfn_log_write)(void*, const char*, const char*, const char*);
    typedef void (*pfn_log_shutdown)(void*);
    typedef int  (*pfn_cache_lookup)(void*, const char*, const char*, char*, int);
    typedef int  (*pfn_cache_put)(void*, const char*, const char*, const char*);

    pfn_create_isolate   g_createIso  = nullptr;
    pfn_detach           g_detach      = nullptr;
    pfn_log_init         g_logInit     = nullptr;
    pfn_log_write        g_logWrite    = nullptr;
    pfn_log_shutdown     g_logShutdown = nullptr;
    pfn_cache_lookup     g_cacheLookup = nullptr;
    pfn_cache_put        g_cachePut    = nullptr;

    // Convert wchar_t* to UTF-8 string for cache keys
    std::string narrow(const wchar_t* w)
    {
        if (!w) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";
        std::string s(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], len, nullptr, nullptr);
        if (!s.empty() && s.back() == '\0') s.pop_back();
        return s;
    }
}

extern "C" {

// Call once; loads log.dll from the same directory as CalcManager.dll.
// Nothing outside this directory is searched, mirroring how the library is shipped.
void calc_log_init()
{
    std::lock_guard<std::mutex> lock(g_initMutex);
    if (g_triedInit) return;
    g_triedInit = true;

    const char* dir = calc_module_dir();
    if (!dir) return;
    std::string full = std::string(dir) + "\\log.dll";
    g_logDll = LoadLibraryA(full.c_str());
    if (!g_logDll) return;

    g_createIso  = (pfn_create_isolate)  GetProcAddress(g_logDll, "graal_create_isolate");
    g_detach     = (pfn_detach)          GetProcAddress(g_logDll, "graal_detach_all_threads_and_tear_down_isolate");
    g_logInit    = (pfn_log_init)        GetProcAddress(g_logDll, "log_init");
    g_logWrite   = (pfn_log_write)       GetProcAddress(g_logDll, "log_write");
    g_logShutdown= (pfn_log_shutdown)    GetProcAddress(g_logDll, "log_shutdown");
    g_cacheLookup= (pfn_cache_lookup)    GetProcAddress(g_logDll, "log_cache_lookup");
    g_cachePut   = (pfn_cache_put)       GetProcAddress(g_logDll, "log_cache_put");

    if (!g_createIso || !g_logInit) return;

    void* isolate = nullptr;
    void* thread = nullptr;
    if (g_createIso(nullptr, &isolate, &thread) != 0) return;
    g_thread = thread;

    g_logInit(g_thread, dir);
}

void calc_log_shutdown()
{
    if (!g_thread) return;
    if (g_logShutdown) g_logShutdown(g_thread);
    if (g_detach) g_detach(g_thread);
    g_thread = nullptr;
}

// Look up a cached result. Returns UTF-8 result string, or null on miss.
// log_cache_lookup returns a status code and writes the text into outBuf, so the
// string is read by scanning for the terminator — the return value is not a length.
const char* calc_cache_lookup(const char* op, const wchar_t* params)
{
    if (!g_thread || !g_cacheLookup) return nullptr;
    static thread_local std::string buf;
    std::string p = narrow(params);
    char raw[65536];
    memset(raw, 0, sizeof(raw));
    int rc = g_cacheLookup(g_thread, op, p.c_str(), raw, sizeof(raw) - 1);
    if (rc < 0) return nullptr;
    raw[sizeof(raw) - 1] = '\0';
    buf.assign(raw);
    if (buf.empty()) return nullptr;
    return buf.c_str();
}

void calc_cache_put(const char* op, const wchar_t* params, const wchar_t* result)
{
    if (!g_thread || !g_cachePut) return;
    std::string p = narrow(params);
    std::string r = narrow(result);
    g_cachePut(g_thread, op, p.c_str(), r.c_str());
}

void calc_log_write(const char* op, const wchar_t* result)
{
    if (!g_enabled || !g_thread || !g_logWrite) return;
    std::string r = narrow(result);
    g_logWrite(g_thread, "BackendMathLibrary", op, r.c_str());
}

// Turn logging on or off without unloading log.dll. The cache keeps working either
// way, so a disabled log only stops the record of operations, not the reuse.
void calc_log_set_enabled(int enable)
{
    g_enabled = (enable != 0);
}

} // extern "C"
