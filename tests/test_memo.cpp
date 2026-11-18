// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// test_memo.cpp — asserts the "计算规律及答案" (calculation rules and answers) table:
// the TMMF baseline data, key normalization, lookup / store / save, and the
// automatic recording that follows an equals command.
// Files are taken from CalcManager.dll's own folder — no absolute path anywhere.
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
    cm_native_init();

    // --- baseline table loads from the folder next to the DLL ---
    int loaded = cm_memo_init();
    check(loaded > 100, L"baseline table has entries");

    // Start from a known state: the runtime table is machine state and may hold
    // entries from an earlier run.
    cm_memo_clear_runtime();
    check(cm_memo_save() == 0, L"runtime table can be cleared and saved");

    const char* base = cm_memo_path(0);
    check(base && strstr(base, "calc_memo.tmmf") != nullptr, L"baseline file is calc_memo.tmmf");
    check(base && strstr(base, "resources\\cmd") != nullptr, L"baseline file lives in resources\\cmd");
    check(base && strstr(base, "wpjc") == nullptr, L"baseline path is not hard-coded");

    // --- well-known answers come straight from the table ---
    char buf[256] = {0};
    check(cm_memo_lookup("pi", buf, sizeof(buf)) == 1, L"lookup pi hits");
    check(strncmp(buf, "3.1415926", 9) == 0, L"pi answer is the stored constant");

    check(cm_memo_lookup("10!", buf, sizeof(buf)) == 1, L"lookup 10! hits");
    check(strcmp(buf, "3628800") == 0, L"10! == 3628800");

    // --- normalization: spacing, full width, operator glyphs ---
    check(cm_memo_lookup("1 + 1", buf, sizeof(buf)) == 1, L"spaces are ignored");
    check(strcmp(buf, "2") == 0, L"1 + 1 == 2");

    check(cm_memo_lookup("9 × 9", buf, sizeof(buf)) == 1, L"multiplication glyph is normalized");
    check(strcmp(buf, "81") == 0, L"9 x 9 == 81");

    check(cm_memo_lookup("２＾１０", buf, sizeof(buf)) == 1, L"full-width digits are normalized");
    check(strcmp(buf, "1024") == 0, L"2^10 == 1024");

    check(cm_memo_lookup("π", buf, sizeof(buf)) == 1, L"pi glyph maps to the pi entry");

    char key[128] = {0};
    cm_memo_normalize("  5 + 3 = ", key, sizeof(key));
    check(strcmp(key, "5+3") == 0, L"normalized key drops spaces and the equals sign");

    // --- an unknown expression misses and can then be learned ---
    check(cm_memo_lookup("2+3", buf, sizeof(buf)) == 0, L"2+3 is not in the baseline");

    check(cm_memo_store("2 + 3", "5") == 0, L"store 2+3 == 5");
    check(cm_memo_lookup("2+3", buf, sizeof(buf)) == 1, L"2+3 hits after store");
    check(strcmp(buf, "5") == 0, L"stored answer is 5");

    int hits = cm_memo_hits("2+3");
    check(hits >= 1, L"hit counter is tracked");

    // --- runtime file is written beside the baseline, not over it ---
    check(cm_memo_save() == 0, L"save writes the runtime table");
    const char* rt = cm_memo_path(1);
    check(rt && strstr(rt, "calc_memo.user.tmmf") != nullptr, L"runtime file is calc_memo.user.tmmf");

    FILE* f = fopen(rt, "rb");
    check(f != nullptr, L"runtime file exists after save");
    if (f)
    {
        std::string all;
        char chunk[512];
        size_t n = 0;
        while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) all.append(chunk, n);
        fclose(f);
        check(all.rfind("#MDRenderer Settings", 0) == 0, L"runtime file keeps the TMMF header");
        check(all.find("2+3=5") != std::string::npos, L"runtime file contains 2+3=5");
    }

    // baseline must not be rewritten by a save
    f = fopen(base, "rb");
    check(f != nullptr, L"baseline file is still readable");
    if (f) fclose(f);

    // --- the engine records a completed expression automatically ---
    cm_memo_set_auto_store(1);
    void* h = cm_engine_new();
    check(h != nullptr, L"engine handle created");
    if (h)
    {
        cm_engine_send_command(h, CM_CMD_CLEAR);
        cm_engine_send_command(h, CM_CMD_7);
        cm_engine_send_command(h, CM_CMD_ADD);
        cm_engine_send_command(h, CM_CMD_9);
        cm_engine_send_command(h, CM_CMD_EQU);

        const wchar_t* disp = cm_engine_get_display(h);
        check(disp && wcscmp(disp, L"16") == 0, L"engine computed 7+9 == 16");

        char answer[128] = {0};
        check(cm_memo_lookup("7+9", answer, sizeof(answer)) == 1, L"equals command recorded 7+9");
        check(strcmp(answer, "16") == 0, L"recorded answer is 16");

        // the suggestion path answers without touching the engine state
        cm_engine_send_command(h, CM_CMD_CLEAR);
        cm_engine_send_command(h, CM_CMD_7);
        cm_engine_send_command(h, CM_CMD_ADD);
        cm_engine_send_command(h, CM_CMD_9);
        char suggest[128] = {0};
        int s = cm_engine_memo_suggest(h, suggest, sizeof(suggest));
        wprintf(L"  info: engine expression = \"%ls\", suggest=%d, answer=\"%hs\"\n",
                cm_engine_get_expression(h), s, suggest);
        check(s == 1, L"suggest answers a known expression");
        check(strcmp(suggest, "16") == 0, L"suggested answer is 16");

        cm_engine_free(h);
    }

    // --- auto-store can be turned off ---
    cm_memo_set_auto_store(0);
    check(cm_memo_get_auto_store() == 0, L"auto-store can be disabled");
    cm_memo_set_auto_store(1);

    wprintf(L"\n%s (failures=%d)\n", g_failures ? L"FAILED" : L"PASSED", g_failures);
    return g_failures ? 1 : 0;
}
