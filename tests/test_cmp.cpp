// test_cmp.cpp — verify the comparison zone (array in / array out).
#include <cstdio>
#include "calc_api.h"

static void dump(void* c)
{
    // item() count isn't exposed separately; print until empty is returned.
    for (int i = 0;; ++i)
    {
        const wchar_t* it = cm_cmp_item(c, i);
        if (!it || !*it) break;
        std::printf("  [%d] %ls\n", i, it);
    }
}

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    void* c = cm_cmp_new();
    if (!c) { std::printf("cm_cmp_new FAILED\n"); return 1; }

    // --- plain compare: 1.5 vs 2, plus a third number for min/max/sum/mean ---
    const wchar_t* plain[] = { L"1.5", L"2", L"0.75" };
    int n = cm_cmp_compare(c, plain, 3);
    std::printf("plain compare (%d items):\n", n);
    dump(c);

    // --- unit compare: 1 kg vs 500 g (unify to kg) ---
    const wchar_t* mass[] = { L"1 kg", L"500 g" };
    n = cm_cmp_compare_units(c, mass, 2);
    std::printf("unit compare mass (%d items):\n", n);
    dump(c);

    // --- unit compare: 0 C vs 32 F (equal after unify to C) ---
    const wchar_t* temp[] = { L"0\u00B0C", L"32\u00B0F" };
    n = cm_cmp_compare_units(c, temp, 2);
    std::printf("unit compare temperature (%d items):\n", n);
    dump(c);

    // --- unit compare: 1 km vs 500 m (unify to km) ---
    const wchar_t* len[] = { L"1 km", L"500 m" };
    n = cm_cmp_compare_units(c, len, 2);
    std::printf("unit compare length (%d items):\n", n);
    dump(c);

    cm_cmp_free(c);
    std::printf("cmp freed\n");
    return 0;
}
