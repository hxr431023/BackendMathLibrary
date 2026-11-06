// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_api.cpp — C export layer over the CalcManager Ratpack arbitrary-precision core.
// Compiled as a DLL. Symbols are extern "C" so any language/runtime can call them.
//
// Logging: instead of writing log files ourselves, we dynamically load the
// user-provided GraalVM-native-image log.dll (see C:\wpjc\md\native\DLL\core\).
// The library creates its own GraalVM isolate and calls log_init/log_write/log_shutdown.
// If log.dll is missing, logging is silently disabled and math calls still work.
#include <windows.h>
#include <string>
#include <cwchar>
#include <cstdio>
#include <cstdlib>
#include <specstrings.h>
#include "Ratpack/ratpak.h"
#include "Ratpack/CalcErr.h"

using namespace std;

static thread_local uint32_t g_radix = 10;
static thread_local int32_t g_precision = 32;
static thread_local bool g_initialized = false;
static thread_local int g_notation = 0; // 0=Float(auto) 1=Scientific 2=Engineering

// ---- Runtime logging via external log.dll (GraalVM native-image) ----
typedef int  (*pfn_graal_create_isolate)(void* params, void** isolate, void** thread);
typedef int  (*pfn_graal_detach_all)(void* thread);
typedef void* (*pfn_graal_get_current_thread)(void* isolate);
typedef int  (*pfn_log_init)(void* thread, char* baseDir);
typedef void (*pfn_log_write)(void* thread, char* params, char* op, char* result);
typedef void (*pfn_log_shutdown)(void* thread);
typedef void (*pfn_log_set_enabled)(void* thread, char* cat, int enabled);

static HMODULE g_log_dll = nullptr;
static void*  g_isolate = nullptr;
static pfn_graal_create_isolate     g_create_isolate_fn = nullptr;
static pfn_graal_detach_all         g_detach_all_fn = nullptr;
static pfn_graal_get_current_thread g_get_thread_fn = nullptr;
static pfn_log_init                 g_log_init_fn = nullptr;
static pfn_log_write                g_log_write_fn = nullptr;
static pfn_log_shutdown             g_log_shutdown_fn = nullptr;
static pfn_log_set_enabled         g_log_set_enabled_fn = nullptr;

// Process-wide autolog switch: when on, every cm_* math call writes one log line.
static bool g_autolog = false;

// wide -> UTF-8, returned buffer must be deleted[] by caller. Never returns null.
static char* w2u(const wchar_t* w)
{
    if (!w)
    {
        char* e = new char[1]; e[0] = 0; return e;
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    char* buf = new char[n];
    WideCharToMultiByte(CP_UTF8, 0, w, -1, buf, n, nullptr, nullptr);
    return buf;
}

static void cm_log2(const wchar_t* fn, const wchar_t* a, const wchar_t* b, const wchar_t* r)
{
    if (!g_autolog || !g_log_write_fn || !g_isolate || !g_get_thread_fn) return;
    void* th = g_get_thread_fn(g_isolate);
    if (!th) return;
    char* cf = w2u(fn);
    char* ca = w2u(a ? a : L"<null>");
    char* cb = w2u(b ? b : L"<null>");
    char crbuf[4096];
    _snprintf(crbuf, sizeof(crbuf), "%s, %s", ca, cb);
    char* cr = w2u(r ? r : L"<err>");
    g_log_write_fn(th, crbuf, cf, cr);
    delete[] cf; delete[] ca; delete[] cb; delete[] cr;
}

static void cm_log1(const wchar_t* fn, const wchar_t* a, const wchar_t* r)
{
    if (!g_autolog || !g_log_write_fn || !g_isolate || !g_get_thread_fn) return;
    void* th = g_get_thread_fn(g_isolate);
    if (!th) return;
    char* cf = w2u(fn);
    char* ca = w2u(a ? a : L"<null>");
    char* cr = w2u(r ? r : L"<err>");
    g_log_write_fn(th, ca, cf, cr);
    delete[] cf; delete[] ca; delete[] cr;
}

// Per-thread lazy init: Ratpak's constants (pi, rat_two, ...) are thread_local,
// so every thread must run ChangeConstants once before doing math.
static void ensure_init()
{
    if (!g_initialized)
    {
        SetDecimalSeparator(L'.');
        ChangeConstants(g_radix, g_precision);
        g_initialized = true;
    }
}

static PRAT parse_rat(const wchar_t* s)
{
    ensure_init();
    if (!s)
        return nullptr;
    wstring str(s);
    bool neg = false;
    if (!str.empty() && str[0] == L'-')
    {
        neg = true;
        str = str.substr(1);
    }
    if (!str.empty() && str[0] == L'+')
    {
        str = str.substr(1);
    }
    return StringToRat(neg, wstring_view(str), false, L"", g_radix, g_precision);
}

static NumberFormat cur_format()
{
    if (g_notation == 1) return NumberFormat::Scientific;
    if (g_notation == 2) return NumberFormat::Engineering;
    return NumberFormat::Float;
}

static wchar_t* rat_to_str(PRAT rat)
{
    if (!rat)
        return nullptr;
    wstring s = RatToString(rat, cur_format(), g_radix, g_precision);
    destroyrat(rat);
    wchar_t* out = new wchar_t[s.size() + 1];
    wcscpy_s(out, s.size() + 1, s.c_str());
    return out;
}

extern "C"
{
    __declspec(dllexport) void cm_init(uint32_t radix, int32_t precision)
    {
        g_radix = radix;
        g_precision = precision;
        g_initialized = false;
    }

    // ---- External log.dll lifecycle ----
    // Load log.dll from logDllPath, create a GraalVM isolate, and call log_init(baseDir).
    // baseDir is an ANSI/UTF-8 path (e.g. "C:/app/data"). Returns 0 on success, -1 on failure.
    __declspec(dllexport) int cm_log_start(const char* logDllPath, const char* baseDir)
    {
        if (g_log_dll) return 0;
        HMODULE dll = LoadLibraryA(logDllPath ? logDllPath : "log.dll");
        if (!dll) return -1;
        g_create_isolate_fn = (pfn_graal_create_isolate)GetProcAddress(dll, "graal_create_isolate");
        g_detach_all_fn     = (pfn_graal_detach_all)GetProcAddress(dll, "graal_detach_all_threads_and_tear_down_isolate");
        g_get_thread_fn     = (pfn_graal_get_current_thread)GetProcAddress(dll, "graal_get_current_thread");
        g_log_init_fn       = (pfn_log_init)GetProcAddress(dll, "log_init");
        g_log_write_fn      = (pfn_log_write)GetProcAddress(dll, "log_write");
        g_log_shutdown_fn   = (pfn_log_shutdown)GetProcAddress(dll, "log_shutdown");
        g_log_set_enabled_fn = (pfn_log_set_enabled)GetProcAddress(dll, "log_set_enabled");
        if (!g_create_isolate_fn || !g_log_init_fn || !g_log_write_fn || !g_log_shutdown_fn)
        {
            FreeLibrary(dll);
            return -1;
        }
        void* mainThread = nullptr;
        if (g_create_isolate_fn(nullptr, &g_isolate, &mainThread) != 0 || !g_isolate)
        {
            FreeLibrary(dll);
            g_isolate = nullptr;
            return -1;
        }
        if (g_log_init_fn(mainThread, (char*)baseDir) != 0)
        {
            FreeLibrary(dll);
            g_isolate = nullptr;
            return -1;
        }
        g_log_dll = dll;
        return 0;
    }

    // log_shutdown (compresses .zst) + tear down isolate + FreeLibrary. Safe to call when not started.
    __declspec(dllexport) void cm_log_stop()
    {
        if (!g_log_dll) return;
        void* th = g_get_thread_fn ? g_get_thread_fn(g_isolate) : nullptr;
        if (th && g_log_shutdown_fn) g_log_shutdown_fn(th);
        if (th && g_detach_all_fn) g_detach_all_fn(th);
        FreeLibrary(g_log_dll);
        g_log_dll = nullptr;
        g_isolate = nullptr;
    }

    // Enable/disable per-call autologging (default off). When on, every cm_* math
    // call writes one line through the loaded log.dll. Settings are process-wide.
    __declspec(dllexport) void cm_set_log_enabled(int enable)
    {
        g_autolog = (enable != 0);
    }

    __declspec(dllexport) wchar_t* cm_add(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        wchar_t* r;
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            r = nullptr;
        }
        else
        {
            addrat(&ra, rb, g_precision);
            destroyrat(rb);
            r = rat_to_str(ra);
        }
        cm_log2(L"cm_add", a, b, r);
        return r;
    }

    __declspec(dllexport) wchar_t* cm_sub(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        wchar_t* r;
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            r = nullptr;
        }
        else
        {
            subrat(&ra, rb, g_precision);
            destroyrat(rb);
            r = rat_to_str(ra);
        }
        cm_log2(L"cm_sub", a, b, r);
        return r;
    }

    __declspec(dllexport) wchar_t* cm_mul(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        wchar_t* r;
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            r = nullptr;
        }
        else
        {
            mulrat(&ra, rb, g_precision);
            destroyrat(rb);
            r = rat_to_str(ra);
        }
        cm_log2(L"cm_mul", a, b, r);
        return r;
    }

    __declspec(dllexport) wchar_t* cm_div(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        wchar_t* r;
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            r = nullptr;
        }
        else
        {
            divrat(&ra, rb, g_precision);
            destroyrat(rb);
            r = rat_to_str(ra);
        }
        cm_log2(L"cm_div", a, b, r);
        return r;
    }

    __declspec(dllexport) wchar_t* cm_pow(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        wchar_t* r;
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            r = nullptr;
        }
        else
        {
            powrat(&ra, rb, g_radix, g_precision);
            destroyrat(rb);
            r = rat_to_str(ra);
        }
        cm_log2(L"cm_pow", a, b, r);
        return r;
    }

    __declspec(dllexport) wchar_t* cm_sqrt(const wchar_t* a)
    {
        PRAT ra = parse_rat(a);
        wchar_t* r;
        if (!ra)
        {
            r = nullptr;
        }
        else
        {
            rootrat(&ra, rat_two, g_radix, g_precision);
            r = rat_to_str(ra);
        }
        cm_log1(L"cm_sqrt", a, r);
        return r;
    }

#define CM_UNARY_TRIG(NAME, FUNC)                                                       \
    __declspec(dllexport) wchar_t* NAME(const wchar_t* a)                              \
    {                                                                                    \
        wchar_t* r;                                                                      \
        try                                                                              \
        {                                                                                \
            PRAT ra = parse_rat(a);                                                      \
            if (!ra) { r = nullptr; }                                                    \
            else { FUNC(&ra, AngleType::Radians, g_radix, g_precision); r = rat_to_str(ra); } \
        }                                                                                \
        catch (...) { r = nullptr; }                                                     \
        cm_log1(L## #NAME, a, r);                                                        \
        return r;                                                                        \
    }

    CM_UNARY_TRIG(cm_sin, sinanglerat)
    CM_UNARY_TRIG(cm_cos, cosanglerat)
    CM_UNARY_TRIG(cm_tan, tananglerat)
    CM_UNARY_TRIG(cm_asin, asinanglerat)
    CM_UNARY_TRIG(cm_acos, acosanglerat)
    CM_UNARY_TRIG(cm_atan, atananglerat)

#define CM_DEG_TRIG(NAME, FUNC)                                                         \
    __declspec(dllexport) wchar_t* NAME(const wchar_t* a)                              \
    {                                                                                    \
        wchar_t* r;                                                                      \
        try                                                                              \
        {                                                                                \
            PRAT ra = parse_rat(a);                                                      \
            if (!ra) { r = nullptr; }                                                    \
            else { FUNC(&ra, AngleType::Degrees, g_radix, g_precision); r = rat_to_str(ra); } \
        }                                                                                \
        catch (...) { r = nullptr; }                                                     \
        cm_log1(L## #NAME, a, r);                                                        \
        return r;                                                                        \
    }

    CM_DEG_TRIG(cm_sin_deg, sinanglerat)
    CM_DEG_TRIG(cm_cos_deg, cosanglerat)
    CM_DEG_TRIG(cm_tan_deg, tananglerat)
    CM_DEG_TRIG(cm_asin_deg, asinanglerat)
    CM_DEG_TRIG(cm_acos_deg, acosanglerat)
    CM_DEG_TRIG(cm_atan_deg, atananglerat)

#define CM_UNARY_MATH(NAME, CODE)                                                        \
    __declspec(dllexport) wchar_t* NAME(const wchar_t* a)                              \
    {                                                                                    \
        wchar_t* r;                                                                      \
        try                                                                              \
        {                                                                                \
            PRAT ra = parse_rat(a);                                                      \
            if (!ra) { r = nullptr; }                                                    \
            else { CODE; r = rat_to_str(ra); }                                           \
        }                                                                                \
        catch (...) { r = nullptr; }                                                     \
        cm_log1(L## #NAME, a, r);                                                        \
        return r;                                                                        \
    }

    CM_UNARY_MATH(cm_ln, lograt(&ra, g_precision))
    CM_UNARY_MATH(cm_log10, log10rat(&ra, g_precision))
    CM_UNARY_MATH(cm_exp, exprat(&ra, g_radix, g_precision))
    CM_UNARY_MATH(cm_fact, factrat(&ra, g_radix, g_precision))
    CM_UNARY_MATH(cm_int, intrat(&ra, g_radix, g_precision))

    __declspec(dllexport) wchar_t* cm_abs(const wchar_t* a)
    {
        wchar_t* r;
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra || !ra->pp || !ra->pq) { r = nullptr; }
            else
            {
                ra->pp->sign = 1;
                ra->pq->sign = 1;
                r = rat_to_str(ra);
            }
        }
        catch (...) { r = nullptr; }
        cm_log1(L"cm_abs", a, r);
        return r;
    }

    __declspec(dllexport) void cm_free(wchar_t* p)
    {
        delete[] p;
    }

    __declspec(dllexport) void cm_set_output_format(uint32_t radix, int32_t precision, int notation)
    {
        g_radix = radix;
        g_precision = precision;
        g_notation = (notation < 0 ? 0 : (notation > 2 ? 2 : notation));
        g_initialized = false;
    }
}
