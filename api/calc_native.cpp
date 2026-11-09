// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_native.cpp — loads config.dll (settings, TMMF) and lerf.dll (language files)
// from the directory that contains CalcManager.dll itself.
//
// Nothing is hard-coded to an absolute path: the module resolves its own directory
// at runtime, so a clone of this repository runs wherever it is placed. If a DLL is
// missing every entry point degrades to a no-op instead of failing the load.

#include <windows.h>
#include <string>
#include <cstring>
#include <fstream>
#include <map>
#include <vector>

namespace {

    HMODULE g_cfgDll  = nullptr;
    HMODULE g_lerfDll = nullptr;
    HMODULE g_tmmfDll = nullptr;
    void*   g_cfgThread  = nullptr;

    typedef int (*pfn_create_isolate)(void*, void**, void**);
    typedef int (*pfn_detach)(void*);

    typedef int  (*pfn_cfg_init)(void*, const char*);
    typedef int  (*pfn_cfg_load)(void*);
    typedef int  (*pfn_cfg_save)(void*);
    typedef int  (*pfn_cfg_get)(void*, const char*, char*, int);
    typedef int  (*pfn_cfg_set)(void*, const char*, const char*);

    typedef int  (*pfn_lerf_validate)(void*, const char*);

    pfn_create_isolate g_createIso = nullptr;
    pfn_detach          g_detach    = nullptr;
    pfn_cfg_init        g_cfgInit   = nullptr;
    pfn_cfg_load        g_cfgLoad   = nullptr;
    pfn_cfg_save        g_cfgSave   = nullptr;
    pfn_cfg_get         g_cfgGet    = nullptr;
    pfn_cfg_set         g_cfgSet    = nullptr;

    std::string g_moduleDir;
    bool        g_nativeInitDone = false;
    std::string g_language = "en";
    std::map<std::string, std::string> g_msgTable;

    std::string narrow_utf8(const wchar_t* w)
    {
        if (!w) return "";
        int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (n <= 0) return "";
        std::string s(n, 0);
        WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, nullptr, nullptr);
        if (!s.empty() && s.back() == '\0') s.pop_back();
        return s;
    }

    void resolve_module_dir()
    {
        if (!g_moduleDir.empty()) return;
        wchar_t path[MAX_PATH] = {0};
        HMODULE self = nullptr;
        BOOL ok = GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)&resolve_module_dir, &self);
        if (ok) GetModuleFileNameW(self, path, MAX_PATH);

        std::string p = narrow_utf8(path);
        size_t pos = p.find_last_of("\\/");
        g_moduleDir = (pos == std::string::npos) ? "." : p.substr(0, pos);
    }

    HMODULE load_beside_module(const char* name)
    {
        resolve_module_dir();
        std::string full = g_moduleDir + "\\" + name;
        return LoadLibraryA(full.c_str());
    }

    // LERF documents use `<id>text<id>` entries; `key=value` and `#` comments are
    // also accepted so a plain key/value file keeps working.
    // lerf.dll is loaded and used for the format check (lerf_validate); the table
    // itself is parsed here because this build of lerf_get echoes the key instead
    // of returning the stored text.
    bool parse_lerf(const std::string& path, std::map<std::string, std::string>& out)
    {
        out.clear();
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;

        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;

            if (line[0] == '<') {
                size_t close = line.find('>');
                if (close == std::string::npos) continue;
                std::string id = line.substr(1, close - 1);
                std::string body = line.substr(close + 1);
                std::string tail = "<" + id + ">";
                size_t end = body.rfind(tail);
                if (end != std::string::npos) body = body.substr(0, end);
                out[id] = body;
                continue;
            }
            size_t eq = line.find('=');
            if (eq != std::string::npos && eq > 0)
                out[line.substr(0, eq)] = line.substr(eq + 1);
        }
        return !out.empty();
    }

    std::string lang_path(const std::string& lang)
    {
        resolve_module_dir();
        return g_moduleDir + "\\resources\\cmd\\" + lang + ".lerf";
    }

    void load_language_table()
    {
        g_msgTable.clear();
        if (parse_lerf(lang_path(g_language), g_msgTable)) return;
        if (g_language != "en") parse_lerf(lang_path("en"), g_msgTable);
    }

    bool init_config()
    {
        if (!g_cfgDll) g_cfgDll = load_beside_module("config.dll");
        if (!g_cfgDll) return false;

        g_createIso = (pfn_create_isolate)GetProcAddress(g_cfgDll, "graal_create_isolate");
        g_detach    = (pfn_detach)        GetProcAddress(g_cfgDll, "graal_detach_all_threads_and_tear_down_isolate");
        g_cfgInit   = (pfn_cfg_init)      GetProcAddress(g_cfgDll, "config_init");
        g_cfgLoad   = (pfn_cfg_load)      GetProcAddress(g_cfgDll, "config_load_settings");
        g_cfgSave   = (pfn_cfg_save)      GetProcAddress(g_cfgDll, "config_save_settings");
        g_cfgGet    = (pfn_cfg_get)       GetProcAddress(g_cfgDll, "config_get_setting");
        g_cfgSet    = (pfn_cfg_set)       GetProcAddress(g_cfgDll, "config_set_setting");
        if (!g_createIso || !g_cfgInit) return false;

        void* isolate = nullptr;
        void* thread  = nullptr;
        if (g_createIso(nullptr, &isolate, &thread) != 0) return false;
        g_cfgThread = thread;

        resolve_module_dir();
        g_cfgInit(g_cfgThread, g_moduleDir.c_str());
        if (g_cfgLoad) g_cfgLoad(g_cfgThread);

        // pick up the stored language before loading the message table.
        // config_get_setting reports a status code and writes into buf; the value
        // is read by scanning for the terminator, never by trusting the return.
        if (g_cfgGet) {
            char buf[256] = {0};
            g_cfgGet(g_cfgThread, "language", buf, sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            if (buf[0] == 'z' && buf[1] == 'h') g_language = "zh";
            else if (buf[0] == 'e' && buf[1] == 'n') g_language = "en";
        }

        return true;
    }

    const char* msg_lookup(const char* id)
    {
        static std::string s;
        if (!id) return "";
        auto it = g_msgTable.find(id);
        s = (it != g_msgTable.end()) ? it->second : std::string(id);
        return s.c_str();
    }

} // namespace

extern "C" {

__declspec(dllexport) int cm_native_init();  // defined below

__declspec(dllexport) const char* calc_module_dir()
{
    resolve_module_dir();
    return g_moduleDir.c_str();
}

// Public alias of the same value (declared in calc_api.h).
__declspec(dllexport) const char* cm_module_dir()
{
    return calc_module_dir();
}

// Internal settings helpers used by calc_api.cpp (not exported as cm_*).
void calc_store_setting(const char* key, const char* value)
{
    if (!g_cfgThread || !g_cfgSet) return;
    g_cfgSet(g_cfgThread, key, value);
}

void calc_store_settings_save()
{
    if (g_cfgThread && g_cfgSave) g_cfgSave(g_cfgThread);
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
        init_config();

        g_lerfDll = load_beside_module("lerf.dll");
        g_tmmfDll = load_beside_module("tmmf.dll");
        // lerf.dll is loaded but not called for validation: every GraalVM DLL owns
        // its own isolate, so a thread pointer created by config.dll is not valid
        // inside lerf.dll. The LERF tables are parsed locally by parse_lerf().
    }
    load_language_table();
    return (g_cfgThread != nullptr) ? 1 : 0;
}

// ---- settings (persisted by config.dll in TMMF format) ----

__declspec(dllexport) int cm_settings_get(const char* key, char* out, int size)
{
    if (!g_cfgThread || !g_cfgGet || !out || size <= 0) return -1;
    return g_cfgGet(g_cfgThread, key, out, size);
}

__declspec(dllexport) int cm_settings_set(const char* key, const char* value)
{
    if (!g_cfgThread || !g_cfgSet) return -1;
    return g_cfgSet(g_cfgThread, key, value);
}

__declspec(dllexport) int cm_settings_save()
{
    if (!g_cfgThread || !g_cfgSave) return -1;
    return g_cfgSave(g_cfgThread);
}

// ---- language (LERF) ----

__declspec(dllexport) int cm_set_language(const char* lang)
{
    if (!lang) return -1;
    g_language = (std::strcmp(lang, "zh") == 0) ? "zh" : "en";
    load_language_table();
    if (g_cfgThread && g_cfgSet) g_cfgSet(g_cfgThread, "language", g_language.c_str());
    if (g_cfgThread && g_cfgSave) g_cfgSave(g_cfgThread);
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
    return msg_lookup(id);
}

// Localized message by id as a wide string (caller must not free).
__declspec(dllexport) const wchar_t* cm_msg_w(const char* id)
{
    static std::wstring w;
    const char* s = cm_msg(id);
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (n <= 0) return L"";
    w.resize(n);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
    if (w.back() == L'\0') w.pop_back();
    return w.c_str();
}

} // extern "C"
