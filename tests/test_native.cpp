// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// test_native.cpp — asserts that the DLL resolves its own directory, loads the
// bundled dependencies, and serves settings (TMMF) and language (LERF) correctly.

#include "calc_api.h"
#include <cstdio>
#include <cstring>
#include <cwchar>

static int g_failures = 0;

static void check(bool cond, const char* what)
{
    if (cond) {
        printf("  ok   %s\n", what);
    } else {
        printf("  FAIL %s\n", what);
        ++g_failures;
    }
}

int main()
{
    printf("[native] init\n");

    // 1. The DLL must report a directory that exists and is its own, not a hard-coded path.
    const char* dir = cm_module_dir();
    check(dir != nullptr && dir[0] != '\0', "cm_module_dir returns a non-empty path");
    check(dir != nullptr && strstr(dir, "wpjc") == nullptr, "module dir is not a hard-coded library path");
    check(GetFileAttributesA(dir) != INVALID_FILE_ATTRIBUTES, "module dir exists on disk");

    char dllPath[MAX_PATH];
    snprintf(dllPath, sizeof(dllPath), "%s\\log.dll", dir ? dir : "");
    check(GetFileAttributesA(dllPath) != INVALID_FILE_ATTRIBUTES, "log.dll sits beside CalcManager.dll");

    // 2. config.dll must be reachable through the bundled copy.
    int native = cm_native_init();
    check(native == 1, "cm_native_init reports config.dll is usable");

    // 3. Settings round-trip through TMMF.
    check(cm_settings_set("radix", "10") == 0, "cm_settings_set accepts a value");
    check(cm_settings_save() == 0, "cm_settings_save writes the TMMF file");

    char buf[128] = {0};
    int rc = cm_settings_get("radix", buf, sizeof(buf));
    printf("       settings_get returned rc=%d value=\"%s\"\n", rc, buf);
    // config_get_setting reports a status code rather than a length; the value it
    // wrote into buf is what the caller actually relies on.
    check(rc >= 0, "cm_settings_get returns a non-negative status");
    check(strcmp(buf, "10") == 0, "cm_settings_get reads back the stored value");

    // 4. Language switching through the LERF tables.
    check(cm_set_language("en") == 0, "cm_set_language accepts en");
    check(strcmp(cm_get_language(), "en") == 0, "current language is en");
    const char* enMsg = cm_msg("msg.ok");
    check(enMsg != nullptr && strcmp(enMsg, "OK") == 0, "en.lerf resolves msg.ok to OK");

    check(cm_set_language("zh") == 0, "cm_set_language accepts zh");
    check(strcmp(cm_get_language(), "zh") == 0, "current language is zh");
    const char* zhMsg = cm_msg("msg.ok");
    check(zhMsg != nullptr && strcmp(zhMsg, "成功") == 0, "zh.lerf resolves msg.ok to Chinese text");
    check(strcmp(cm_msg("does.not.exist"), "does.not.exist") == 0, "unknown id falls back to the id");

    // 5. The arithmetic API keeps working with everything loaded.
    wchar_t* sum = cm_add(L"1", L"2");
    check(sum != nullptr && wcscmp(sum, L"3") == 0, "cm_add(1,2) == 3");
    if (sum) cm_free(sum);

    cm_set_language("en");

    printf("\n[native] failures: %d\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
