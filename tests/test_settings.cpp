// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// test_settings.cpp — asserts runtime settings: external log.dll autologging +
// output format (radix / precision / notation).
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <string>
#include <direct.h>
#include "calc_api.h"

static int g_failures = 0;

static void check(bool cond, const wchar_t* msg)
{
    if (cond) wprintf(L"  ok: %ls\n", msg);
    else { wprintf(L"  FAIL: %ls\n", msg); g_failures++; }
}

static bool dir_has_zst(const char* dir)
{
    std::string pat = std::string(dir) + "/*.zst";
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    FindClose(h);
    return true;
}

int wmain()
{
    cm_init(10, 32);

    // --- external log.dll autologging ---
    const char* kLogDll = "C:/wpjc/md/native/DLL/core/log.dll";
    const char* kBaseDir = "test_log_data";
    _mkdir(kBaseDir);

    int rc = cm_log_start(kLogDll, kBaseDir);
    check(rc == 0, L"cm_log_start loads log.dll and inits");
    if (rc == 0)
    {
        cm_set_log_enabled(1);
        wchar_t* r = cm_add(L"2", L"3");
        check(r && wcscmp(r, L"5") == 0, L"add(2,3) == 5");
        if (r) cm_free(r);
        r = cm_sqrt(L"16");
        check(r && wcscmp(r, L"4") == 0, L"sqrt(16) == 4");
        if (r) cm_free(r);
        cm_set_log_enabled(0);
        cm_log_stop();  // compress back to .zst
        check(dir_has_zst("test_log_data/log"), L"log_shutdown produced a .zst archive");
    }

    // --- output format: notation / radix / precision ---
    cm_set_output_format(10, 32, 0);
    wchar_t* r = cm_add(L"0.5", L"0.25");
    check(r && wcscmp(r, L"0.75") == 0, L"float: 0.5+0.25 == 0.75");
    if (r) cm_free(r);

    cm_set_output_format(16, 8, 0);
    r = cm_add(L"A", L"5");
    check(r && wcscmp(r, L"F") == 0, L"hex: A+5 == F");
    if (r) cm_free(r);

    cm_set_output_format(2, 8, 0);
    r = cm_add(L"101", L"1");
    check(r && wcscmp(r, L"110") == 0, L"bin: 101+1 == 110");
    if (r) cm_free(r);

    cm_set_output_format(10, 3, 1);
    r = cm_mul(L"123456", L"1000");
    check(r != nullptr, L"sci notation returns a result");
    if (r) cm_free(r);

    wprintf(L"%s (failures=%d)\n", g_failures ? L"FAILED" : L"PASSED", g_failures);
    return g_failures ? 1 : 0;
}
