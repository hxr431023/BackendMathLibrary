// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// test_settings.cpp — asserts the new runtime settings: per-call logging and
// output format (radix / precision / notation).
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <string>
#include <fstream>
#include "calc_api.h"

static int g_failures = 0;

static void check(bool cond, const wchar_t* msg)
{
    if (cond)
    {
        wprintf(L"  ok: %ls\n", msg);
    }
    else
    {
        wprintf(L"  FAIL: %ls\n", msg);
        g_failures++;
    }
}

int wmain()
{
    cm_init(10, 32);

    // --- per-call logging ---
    cm_set_log_file(L"test_settings.log");
    cm_set_log_enabled(1);
    wchar_t* r = cm_add(L"2", L"3");
    check(r && wcscmp(r, L"5") == 0, L"add(2,3) == 5");
    if (r) cm_free(r);
    r = cm_sqrt(L"16");
    check(r && wcscmp(r, L"4") == 0, L"sqrt(16) == 4");
    if (r) cm_free(r);
    cm_set_log_enabled(0);
    cm_set_log_file(nullptr); // back to stderr, close handle

    std::ifstream f("test_settings.log");
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    check(content.find("cm_add(2, 3) = 5") != std::string::npos, L"log has cm_add(2, 3) = 5");
    check(content.find("cm_sqrt(16) = 4") != std::string::npos, L"log has cm_sqrt(16) = 4");
    // logging disabled now: a call must NOT append a new line
    std::size_t before = content.size();
    r = cm_add(L"9", L"9");
    if (r) cm_free(r);
    std::ifstream f2("test_settings.log");
    std::string content2((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());
    check(content2.size() == before, L"log unchanged while disabled");

    // --- output format: notation / radix / precision ---
    cm_set_output_format(10, 32, 0); // float
    r = cm_add(L"0.5", L"0.25");
    check(r && wcscmp(r, L"0.75") == 0, L"float: 0.5+0.25 == 0.75");
    if (r) cm_free(r);

    cm_set_output_format(16, 8, 0); // hexadecimal
    r = cm_add(L"A", L"5");
    check(r && wcscmp(r, L"F") == 0, L"hex: A+5 == F");
    if (r) cm_free(r);

    cm_set_output_format(2, 8, 0); // binary
    r = cm_add(L"101", L"1");
    check(r && wcscmp(r, L"110") == 0, L"bin: 101+1 == 110");
    if (r) cm_free(r);

    cm_set_output_format(10, 3, 1); // scientific, 3 digits
    r = cm_mul(L"123456", L"1000");
    check(r != nullptr, L"sci notation returns a result");
    if (r) cm_free(r);

    wprintf(L"%s (failures=%d)\n", g_failures ? L"FAILED" : L"PASSED", g_failures);
    return g_failures ? 1 : 0;
}
