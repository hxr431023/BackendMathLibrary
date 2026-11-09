// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_api.cpp — C export layer over the CalcManager Ratpack arbitrary-precision core.
// Compiled as a DLL. Symbols are extern "C" so any language/runtime can call them.
#include <string>
#include <cwchar>
#include <cstdio>
#include <windows.h>
#include <specstrings.h>
#include "Ratpack/ratpak.h"
#include "Ratpack/CalcErr.h"

using namespace std;

// ---- log.dll cache integration (declared in calc_log.cpp) ----
extern "C" {
    void calc_log_init();
    void calc_log_shutdown();
    const char* calc_cache_lookup(const char* op, const wchar_t* params);
    void calc_cache_put(const char* op, const wchar_t* params, const wchar_t* result);
    void calc_log_write(const char* op, const wchar_t* result);
    // settings persistence (implemented in calc_native.cpp)
    void calc_store_setting(const char* key, const char* value);
    void calc_store_settings_save();
}

static wchar_t* utf8_to_wchar(const char* s)
{
    if (!s) return nullptr;
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (len <= 0) return nullptr;
    wchar_t* out = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, s, -1, out, len);
    return out;
}

// Join two wide strings with a pipe separator for cache key
static string join_params(const wchar_t* a, const wchar_t* b)
{
    string sa, sb;
    if (a) {
        int n = WideCharToMultiByte(CP_UTF8, 0, a, -1, nullptr, 0, nullptr, nullptr);
        sa.resize(n > 0 ? n - 1 : 0);
        if (n > 0) WideCharToMultiByte(CP_UTF8, 0, a, -1, &sa[0], n, nullptr, nullptr);
    }
    if (b) {
        int n = WideCharToMultiByte(CP_UTF8, 0, b, -1, nullptr, 0, nullptr, nullptr);
        sb.resize(n > 0 ? n - 1 : 0);
        if (n > 0) WideCharToMultiByte(CP_UTF8, 0, b, -1, &sb[0], n, nullptr, nullptr);
    }
    return sa + "|" + sb;
}

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
        // log.dll is loaded from this module's own directory; without this the
        // cache lookup and the write of every operation are silent no-ops.
        calc_log_init();
    }

    // Settings entry point used by hosts that drive radix / precision / notation
    // at runtime. It applies to the calling thread and persists the default in TMMF.
    __declspec(dllexport) void cm_set_output_format(uint32_t radix, int32_t precision, int scientific)
    {
        g_radix = radix;
        g_precision = precision;
        g_initialized = false;

        char buf[64];
        snprintf(buf, sizeof(buf), "%u", radix);
        calc_store_setting("radix", buf);
        snprintf(buf, sizeof(buf), "%d", precision);
        calc_store_setting("precision", buf);
        calc_store_setting("scientific", scientific ? "1" : "0");
        calc_store_settings_save();
    }

    __declspec(dllexport) wchar_t* cm_add(const wchar_t* a, const wchar_t* b)
    {
        // Cache key: "a|b"
        wstring key = wstring(a ? a : L"") + L"|" + (b ? b : L"");
        if (const char* hit = calc_cache_lookup("cm_add", key.c_str())) {
            return utf8_to_wchar(hit);
        }

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
        wchar_t* result = rat_to_str(ra);
        calc_cache_put("cm_add", key.c_str(), result);
        calc_log_write("cm_add", result);
        return result;
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
