// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_api.cpp — C export layer over the CalcManager Ratpack arbitrary-precision core.
// Compiled as a DLL. Symbols are extern "C" so any language/runtime can call them.
#include <string>
#include <cwchar>
#include <cstdio>
#include <specstrings.h>
#include "Ratpack/ratpak.h"
#include "Ratpack/CalcErr.h"

using namespace std;

static thread_local uint32_t g_radix = 10;
static thread_local int32_t g_precision = 32;
static thread_local bool g_initialized = false;
static thread_local int g_notation = 0; // 0=Float(auto) 1=Scientific 2=Engineering

// Per-call logging state (thread_local so concurrent threads never share a cursor).
static thread_local bool g_log_enabled = false;
static thread_local wstring g_log_path;   // empty -> stderr
static thread_local FILE* g_log_file = nullptr; // lazily opened handle (append)

// Per-thread lazy init: Ratpak's constants (pi, rat_two, ...) are thread_local,
// so every thread must run ChangeConstants once before doing math. cm_init just
// records the radix/precision and lets the next call re-initialize on this thread.
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

// ---- logging helpers ----
static FILE* log_stream()
{
    if (g_log_file) return g_log_file;
    if (g_log_path.empty()) return stderr;
    g_log_file = _wfopen(g_log_path.c_str(), L"a, ccs=UTF-8");
    return g_log_file ? g_log_file : stderr;
}

static void cm_log2(const wchar_t* fn, const wchar_t* a, const wchar_t* b, const wchar_t* r)
{
    if (!g_log_enabled) return;
    FILE* f = log_stream();
    fwprintf(f, L"[cm] %ls(%ls, %ls) = %ls\n", fn, a ? a : L"<null>", b ? b : L"<null>", r ? r : L"<err>");
    fflush(f);
}

static void cm_log1(const wchar_t* fn, const wchar_t* a, const wchar_t* r)
{
    if (!g_log_enabled) return;
    FILE* f = log_stream();
    fwprintf(f, L"[cm] %ls(%ls) = %ls\n", fn, a ? a : L"<null>", r ? r : L"<err>");
    fflush(f);
}

extern "C"
{
    __declspec(dllexport) void cm_init(uint32_t radix, int32_t precision)
    {
        g_radix = radix;
        g_precision = precision;
        g_initialized = false;
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

    __declspec(dllexport) void cm_set_log_enabled(int enable)
    {
        g_log_enabled = (enable != 0);
    }

    __declspec(dllexport) int cm_set_log_file(const wchar_t* path)
    {
        if (g_log_file)
        {
            fclose(g_log_file);
            g_log_file = nullptr;
        }
        if (path && path[0])
        {
            g_log_path = path;
        }
        else
        {
            g_log_path.clear();
        }
        return 0;
    }

    __declspec(dllexport) void cm_set_output_format(uint32_t radix, int32_t precision, int notation)
    {
        g_radix = radix;
        g_precision = precision;
        g_notation = (notation < 0 ? 0 : (notation > 2 ? 2 : notation));
        g_initialized = false; // constants re-derived on the next math call on this thread
    }
}
