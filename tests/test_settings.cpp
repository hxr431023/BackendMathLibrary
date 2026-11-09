// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// test_settings.cpp — asserts the runtime settings: bundled log.dll logging switch,
// output format (radix / precision / notation) and the TMMF persistence behind them.
// No absolute library path is used: log.dll is taken from CalcManager.dll's own folder.
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <string>
#include "calc_api.h"

static int g_failures = 0;

static void check(bool cond, const wchar_t* msg)
{
    if (cond) wprintf(L"  ok: %ls\n", msg);
    else { wprintf(L"  FAIL: %ls\n", msg); g_failures++; }
}

int wmain()
{
    cm_init(10, 32);

    // --- bundled log.dll: loaded from the module directory, no path argument ---
    const char* dir = cm_module_dir();
    check(dir && dir[0] != '\0', L"module directory is known");
    check(dir && strstr(dir, "wpjc") == nullptr, L"module directory is not a hard-coded library path");

    int rc = cm_log_start();
    check(rc == 0, L"cm_log_start opens the bundled log.dll");

    cm_set_log_enabled(1);
    wchar_t* r = cm_add(L"2", L"3");
    check(r && wcscmp(r, L"5") == 0, L"logging on: add(2,3) == 5");
    if (r) cm_free(r);

    r = cm_sqrt(L"16");
    check(r && wcscmp(r, L"4") == 0, L"logging on: sqrt(16) == 4");
    if (r) cm_free(r);

    cm_set_log_enabled(0);
    cm_log_stop();

    // --- output format: notation / radix / precision ---
    cm_set_output_format(10, 32, 0);
    r = cm_add(L"0.5", L"0.25");
    check(r && wcscmp(r, L"0.75") == 0, L"decimal: 0.5+0.25 == 0.75");
    if (r) cm_free(r);

    cm_set_output_format(16, 8, 0);
    r = cm_add(L"A", L"5");
    check(r && wcscmp(r, L"F") == 0, L"hex: A+5 == F");
    if (r) cm_free(r);

    cm_set_output_format(2, 8, 0);
    r = cm_add(L"101", L"1");
    check(r && wcscmp(r, L"110") == 0, L"binary: 101+1 == 110");
    if (r) cm_free(r);

    cm_set_output_format(10, 3, 1);
    r = cm_mul(L"123456", L"1000");
    check(r != nullptr, L"scientific notation still returns a result");
    if (r) cm_free(r);

    // --- the settings above are persisted in the TMMF file ---
    char buf[128] = {0};
    int got = cm_settings_get("radix", buf, sizeof(buf));
    check(got >= 0, L"radix was persisted");
    check(strcmp(buf, "10") == 0, L"persisted radix is 10");

    wprintf(L"\n%s (failures=%d)\n", g_failures ? L"FAILED" : L"PASSED", g_failures);
    return g_failures ? 1 : 0;
}
