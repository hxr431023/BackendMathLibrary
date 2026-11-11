// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_native.cpp — the calc-specific native layer: cm_* exports, the active
// language and its message table, and the log switch.
//
// The reusable half (module directory, dependency DLL loading, TMMF settings,
// LERF parsing, UTF-8 conversion) lives in native_runtime.h/.cpp so another
// project can take that pair of files and get the same behaviour.
//
// Nothing is hard-coded to an absolute path: the module resolves its own directory
// at runtime, so a clone of this repository runs wherever it is placed. If a DLL is
// missing every entry point degrades to a no-op instead of failing the load.

#include "native_runtime.h"

#include <cstring>
#include <map>
#include <string>

namespace {

    HMODULE g_lerfDll = nullptr;
    HMODULE g_tmmfDll = nullptr;

    std::string g_language = "en";
    std::map<std::string, std::string> g_msgTable;
    bool g_nativeInitDone = false;

    void load_language_table()
    {
        bm_runtime::load_lerf_table(g_language.c_str(), g_msgTable);
    }

    void pick_stored_language()
    {
        // settings_get reports a status code and writes into the buffer, so the
        // value is read from the buffer, never from the return value.
        char buf[256] = {0};
        if (bm_runtime::settings_get("language", buf, sizeof(buf)) < 0) return;
        buf[sizeof(buf) - 1] = '\0';
        if (buf[0] == 'z' && buf[1] == 'h') g_language = "zh";
        else if (buf[0] == 'e' && buf[1] == 'n') g_language = "en";
    }

} // namespace

extern "C" {

__declspec(dllexport) int cm_native_init();  // defined below

// Kept because calc_log.cpp resolves its log directory through this name.
__declspec(dllexport) const char* calc_module_dir()
{
    return bm_runtime::module_dir();
}

// Public alias of the same value (declared in calc_api.h).
__declspec(dllexport) const char* cm_module_dir()
{
    return bm_runtime::module_dir();
}

// Internal settings helpers used by calc_api.cpp (not exported as cm_*).
void calc_store_setting(const char* key, const char* value)
{
    bm_runtime::settings_set(key, value);
}

void calc_store_settings_save()
{
    bm_runtime::settings_save();
}

// ---- logging switch (wraps calc_log.cpp) ----
extern void calc_log_init();
extern void calc_log_shutdown();
extern void calc_log_set_enabled(int enable);

__declspec(dllexport) int cm_log_start()
{
    cm_native_init();
    calc_log_init();
    return 0;
}

__declspec(dllexport) void cm_log_stop()
{
    calc_log_set_enabled(0);
    calc_log_shutdown();
}

__declspec(dllexport) void cm_set_log_enabled(int enable)
{
    calc_log_init();
    calc_log_set_enabled(enable);
    calc_store_setting("log_enabled", enable ? "1" : "0");
    calc_store_settings_save();
}

// Load the native dependencies that live next to CalcManager.dll.
// Safe to call repeatedly; returns 1 when config.dll is usable.
__declspec(dllexport) int cm_native_init()
{
    if (!g_nativeInitDone) {
        g_nativeInitDone = true;
        if (bm_runtime::settings_init()) pick_stored_language();

        g_lerfDll = bm_runtime::load_beside("lerf.dll");
        g_tmmfDll = bm_runtime::load_beside("tmmf.dll");
        // lerf.dll is loaded but not called for validation: every GraalVM DLL owns
        // its own isolate, so a thread pointer created by config.dll is not valid
        // inside lerf.dll. The LERF tables are parsed by bm_runtime::parse_lerf_file.
    }
    load_language_table();
    return bm_runtime::settings_ready() ? 1 : 0;
}

// ---- settings (persisted by config.dll in TMMF format) ----

__declspec(dllexport) int cm_settings_get(const char* key, char* out, int size)
{
    return bm_runtime::settings_get(key, out, size);
}

__declspec(dllexport) int cm_settings_set(const char* key, const char* value)
{
    return bm_runtime::settings_set(key, value);
}

__declspec(dllexport) int cm_settings_save()
{
    return bm_runtime::settings_save();
}

// ---- language (LERF) ----

__declspec(dllexport) int cm_set_language(const char* lang)
{
    if (!lang) return -1;
    g_language = (std::strcmp(lang, "zh") == 0) ? "zh" : "en";
    load_language_table();
    bm_runtime::settings_set("language", g_language.c_str());
    bm_runtime::settings_save();
    return 0;
}

__declspec(dllexport) const char* cm_get_language()
{
    return g_language.c_str();
}

// Localized message by id, UTF-8. Falls back to the id itself.
__declspec(dllexport) const char* cm_msg(const char* id)
{
    if (g_msgTable.empty()) load_language_table();
    return bm_runtime::lerf_lookup(g_msgTable, id);
}

// Localized message by id as a wide string (caller must not free).
__declspec(dllexport) const wchar_t* cm_msg_w(const char* id)
{
    static std::wstring w;
    w = bm_runtime::wide_from_utf8(cm_msg(id));
    return w.c_str();
}

} // extern "C"
