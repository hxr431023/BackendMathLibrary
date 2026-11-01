// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_api.cpp — C export layer over the CalcManager Ratpack arbitrary-precision core.
// Compiled as a DLL. Symbols are extern "C" so any language/runtime can call them.
#include <string>
#include <cwchar>
#include <specstrings.h>
#include "Ratpack/ratpak.h"
#include "Ratpack/CalcErr.h"

using namespace std;

static thread_local uint32_t g_radix = 10;
static thread_local int32_t g_precision = 32;
static thread_local bool g_initialized = false;

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

static wchar_t* rat_to_str(PRAT rat)
{
    if (!rat)
        return nullptr;
    wstring s = RatToString(rat, NumberFormat::Float, g_radix, g_precision);
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

    __declspec(dllexport) wchar_t* cm_add(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            return nullptr;
        }
        addrat(&ra, rb, g_precision);
        destroyrat(rb);
        return rat_to_str(ra);
    }

    __declspec(dllexport) wchar_t* cm_sub(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            return nullptr;
        }
        subrat(&ra, rb, g_precision);
        destroyrat(rb);
        return rat_to_str(ra);
    }

    __declspec(dllexport) wchar_t* cm_mul(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            return nullptr;
        }
        mulrat(&ra, rb, g_precision);
        destroyrat(rb);
        return rat_to_str(ra);
    }

    __declspec(dllexport) wchar_t* cm_div(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            return nullptr;
        }
        divrat(&ra, rb, g_precision);
        destroyrat(rb);
        return rat_to_str(ra);
    }

    __declspec(dllexport) wchar_t* cm_pow(const wchar_t* a, const wchar_t* b)
    {
        PRAT ra = parse_rat(a);
        PRAT rb = parse_rat(b);
        if (!ra || !rb)
        {
            if (ra) destroyrat(ra);
            if (rb) destroyrat(rb);
            return nullptr;
        }
        powrat(&ra, rb, g_radix, g_precision);
        destroyrat(rb);
        return rat_to_str(ra);
    }

    __declspec(dllexport) wchar_t* cm_sqrt(const wchar_t* a)
    {
        PRAT ra = parse_rat(a);
        if (!ra)
            return nullptr;
        rootrat(&ra, rat_two, g_radix, g_precision);
        return rat_to_str(ra);
    }

    __declspec(dllexport) wchar_t* cm_sin(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            sinanglerat(&ra, AngleType::Radians, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_cos(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            cosanglerat(&ra, AngleType::Radians, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_tan(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            tananglerat(&ra, AngleType::Radians, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_asin(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            asinanglerat(&ra, AngleType::Radians, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_acos(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            acosanglerat(&ra, AngleType::Radians, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_atan(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            atananglerat(&ra, AngleType::Radians, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

#define CM_DEG_TRIG(NAME, FUNC)                                                     \
    __declspec(dllexport) wchar_t* NAME(const wchar_t* a)                          \
    {                                                                               \
        try                                                                         \
        {                                                                           \
            PRAT ra = parse_rat(a);                                                 \
            if (!ra) return nullptr;                                                \
            FUNC(&ra, AngleType::Degrees, g_radix, g_precision);                    \
            return rat_to_str(ra);                                                  \
        }                                                                           \
        catch (...) { return nullptr; }                                             \
    }

    CM_DEG_TRIG(cm_sin_deg, sinanglerat)
    CM_DEG_TRIG(cm_cos_deg, cosanglerat)
    CM_DEG_TRIG(cm_tan_deg, tananglerat)
    CM_DEG_TRIG(cm_asin_deg, asinanglerat)
    CM_DEG_TRIG(cm_acos_deg, acosanglerat)
    CM_DEG_TRIG(cm_atan_deg, atananglerat)

    __declspec(dllexport) wchar_t* cm_ln(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            lograt(&ra, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_log10(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            log10rat(&ra, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_exp(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            exprat(&ra, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_fact(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            factrat(&ra, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_abs(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra || !ra->pp || !ra->pq) return nullptr;
            ra->pp->sign = 1;
            ra->pq->sign = 1;
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) wchar_t* cm_int(const wchar_t* a)
    {
        try
        {
            PRAT ra = parse_rat(a);
            if (!ra) return nullptr;
            intrat(&ra, g_radix, g_precision);
            return rat_to_str(ra);
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) void cm_free(wchar_t* p)
    {
        delete[] p;
    }
}
