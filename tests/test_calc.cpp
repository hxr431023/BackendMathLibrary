// test_calc.cpp — loads CalcManager.dll via import lib and sanity-checks the C API.
#include <cstdio>
#include <cwchar>
#include <cstring>

extern "C"
{
    __declspec(dllimport) void cm_init(unsigned int radix, int precision);
    __declspec(dllimport) wchar_t* cm_add(const wchar_t* a, const wchar_t* b);
    __declspec(dllimport) wchar_t* cm_sub(const wchar_t* a, const wchar_t* b);
    __declspec(dllimport) wchar_t* cm_mul(const wchar_t* a, const wchar_t* b);
    __declspec(dllimport) wchar_t* cm_div(const wchar_t* a, const wchar_t* b);
    __declspec(dllimport) wchar_t* cm_pow(const wchar_t* a, const wchar_t* b);
    __declspec(dllimport) wchar_t* cm_sqrt(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_sin(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_cos(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_tan(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_asin(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_acos(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_atan(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_ln(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_log10(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_exp(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_fact(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_abs(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_int(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_sin_deg(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_cos_deg(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_tan_deg(const wchar_t* a);
    __declspec(dllimport) wchar_t* cm_atan_deg(const wchar_t* a);
    __declspec(dllimport) void cm_free(wchar_t* p);
}

int main()
{
    cm_init(10, 32);
    wprintf(L"init done (radix=10, precision=32)\n");

    struct { const char* op; const wchar_t* a; const wchar_t* b; } cases[] = {
        { "add",  L"1.5",   L"2.25" },
        { "sub",  L"10",    L"3.5"  },
        { "mul",  L"3",     L"7"    },
        { "div",  L"10",    L"4"    },
        { "pow",  L"2",     L"10"   },
    };

    for (auto& c : cases)
    {
        wchar_t* r = nullptr;
        if      (strcmp(c.op, "add") == 0) r = cm_add(c.a, c.b);
        else if (strcmp(c.op, "sub") == 0) r = cm_sub(c.a, c.b);
        else if (strcmp(c.op, "mul") == 0) r = cm_mul(c.a, c.b);
        else if (strcmp(c.op, "div") == 0) r = cm_div(c.a, c.b);
        else if (strcmp(c.op, "pow") == 0) r = cm_pow(c.a, c.b);
        wprintf(L"%ls %S %ls = %ls\n", c.a, c.op, c.b, r);
        cm_free(r);
    }

    wchar_t* s = cm_sqrt(L"2");
    wprintf(L"sqrt(2) = %ls\n", s);
    cm_free(s);

    wchar_t* big = cm_pow(L"2", L"100");
    wprintf(L"2^100 (arbitrary precision) = %ls\n", big);
    cm_free(big);

    typedef wchar_t* (*unary_fn)(const wchar_t*);
    struct { const char* name; unary_fn fn; const wchar_t* arg; } u[] = {
        { "sin",   cm_sin,   L"0"    },
        { "cos",   cm_cos,   L"0"    },
        { "asin",  cm_asin,  L"0"    },
        { "ln",    cm_ln,    L"1"    },
        { "log10", cm_log10, L"100"  },
        { "exp",   cm_exp,   L"0"    },
        { "fact",  cm_fact,  L"10"   },
        { "abs",   cm_abs,   L"-3.5" },
        { "int",   cm_int,   L"3.7"  },
        { "sqrt",  cm_sqrt,  L"2"    },
        { "sin_deg", cm_sin_deg, L"30" }, // 0.5
        { "cos_deg", cm_cos_deg, L"0"  }, // 1
        { "tan_deg", cm_tan_deg, L"45" }, // 1
        { "atan_deg", cm_atan_deg, L"1" }, // 45
    };
    for (auto& x : u)
    {
        wchar_t* r = x.fn(x.arg);
        wprintf(L"%hs(%ls) = %ls\n", x.name, x.arg, r);
        cm_free(r);
    }

    return 0;
}
