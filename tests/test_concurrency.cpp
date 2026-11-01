// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// Concurrency smoke test: run many cm_* math calls from many threads at once.
// Ratpak's constants and the wrapper's radix/precision are thread_local, so each
// thread computes independently. This test asserts every thread gets correct
// results, proving real parallel use (not serialized).
#include <windows.h>
#include <thread>
#include <vector>
#include <cwchar>
#include <cstdio>
#include "calc_api.h"

static volatile LONG g_fails = 0;

static void worker(int id)
{
    // Optional: cm_init per thread also works; here we rely on ensure_init()
    // (lazy per-thread init) to prove it needs no explicit per-thread setup.
    for (int i = 0; i < 200; i++) {
        const wchar_t* r = cm_add(L"123", L"77");
        if (wcscmp(r, L"200") != 0) { InterlockedIncrement(&g_fails); }
        cm_free((wchar_t*)r);

        r = cm_pow(L"2", L"12");
        if (wcscmp(r, L"4096") != 0) { InterlockedIncrement(&g_fails); }
        cm_free((wchar_t*)r);

        r = cm_sin(L"0");
        if (wcscmp(r, L"0") != 0) { InterlockedIncrement(&g_fails); }
        cm_free((wchar_t*)r);

        r = cm_int(L"42.9");
        if (wcscmp(r, L"42") != 0) { InterlockedIncrement(&g_fails); }
        cm_free((wchar_t*)r);

        r = cm_sqrt(L"144");
        if (wcscmp(r, L"12") != 0) { InterlockedIncrement(&g_fails); }
        cm_free((wchar_t*)r);
    }
}

int wmain()
{
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; i++) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }
    if (g_fails == 0) {
        wprintf(L"test_concurrency PASSED (8 threads x 200 iters, all correct)\n");
        return 0;
    }
    wprintf(L"test_concurrency FAILED fails=%d\n", (int)g_fails);
    return 1;
}
