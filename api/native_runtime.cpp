// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// native_runtime.cpp — module directory, dependency DLLs, TMMF settings, LERF.

#include "native_runtime.h"

#include <cstring>
#include <fstream>

namespace bm_runtime {
namespace {

    HMODULE g_cfgDll    = nullptr;
    void*   g_cfgThread = nullptr;

    typedef int (*pfn_create_isolate)(void*, void**, void**);
    typedef int (*pfn_cfg_init)(void*, const char*);
    typedef int (*pfn_cfg_load)(void*);
    typedef int (*pfn_cfg_save)(void*);
    typedef int (*pfn_cfg_get)(void*, const char*, char*, int);
    typedef int (*pfn_cfg_set)(void*, const char*, const char*);

    pfn_cfg_init g_cfgInit = nullptr;
    pfn_cfg_load g_cfgLoad = nullptr;
    pfn_cfg_save g_cfgSave = nullptr;
    pfn_cfg_get  g_cfgGet  = nullptr;
    pfn_cfg_set  g_cfgSet  = nullptr;

    std::string g_moduleDir;
    bool        g_cfgInitDone = false;

    // Resolving the directory goes through this function's own address, so it
    // must stay non-inlined: that address is what identifies the module.
    HMODULE resolve_self_module()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)&resolve_self_module, &self);
        return self;
    }

} // namespace

std::string utf8_from_wide(const wchar_t* w)
{
    if (!w) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return "";
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, nullptr, nullptr);
    if (!s.empty() && s.back() == '\0') s.pop_back();
    return s;
}

std::wstring wide_from_utf8(const char* s)
{
    if (!s) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    if (n <= 0) return L"";
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

const char* module_dir()
{
    if (!g_moduleDir.empty()) return g_moduleDir.c_str();

    wchar_t path[MAX_PATH] = {0};
    HMODULE self = resolve_self_module();
    if (self) GetModuleFileNameW(self, path, MAX_PATH);

    std::string p = utf8_from_wide(path);
    size_t pos = p.find_last_of("\\/");
    g_moduleDir = (pos == std::string::npos) ? "." : p.substr(0, pos);
    return g_moduleDir.c_str();
}

std::string module_path(const char* leaf)
{
    std::string full = module_dir();
    full += "\\";
    full += leaf ? leaf : "";
    return full;
}

HMODULE load_beside(const char* name)
{
    if (!name) return nullptr;
    std::string full = module_path(name);
    return LoadLibraryA(full.c_str());
}

bool settings_init()
{
    if (g_cfgInitDone) return g_cfgThread != nullptr;
    g_cfgInitDone = true;

    if (!g_cfgDll) g_cfgDll = load_beside("config.dll");
    if (!g_cfgDll) return false;

    pfn_create_isolate createIso =
        (pfn_create_isolate)GetProcAddress(g_cfgDll, "graal_create_isolate");
    g_cfgInit = (pfn_cfg_init)GetProcAddress(g_cfgDll, "config_init");
    g_cfgLoad = (pfn_cfg_load)GetProcAddress(g_cfgDll, "config_load_settings");
    g_cfgSave = (pfn_cfg_save)GetProcAddress(g_cfgDll, "config_save_settings");
    g_cfgGet  = (pfn_cfg_get)GetProcAddress(g_cfgDll, "config_get_setting");
    g_cfgSet  = (pfn_cfg_set)GetProcAddress(g_cfgDll, "config_set_setting");
    if (!createIso || !g_cfgInit) return false;

    void* isolate = nullptr;
    void* thread  = nullptr;
    if (createIso(nullptr, &isolate, &thread) != 0) return false;
    g_cfgThread = thread;

    g_cfgInit(g_cfgThread, module_dir());
    // config_load_settings is what reads the TMMF file back; without it every
    // lookup reports the key as absent.
    if (g_cfgLoad) g_cfgLoad(g_cfgThread);
    return true;
}

bool settings_ready()
{
    return g_cfgThread != nullptr;
}

int settings_get(const char* key, char* out, int size)
{
    if (!g_cfgThread || !g_cfgGet || !out || size <= 0) return -1;
    // The DLL reports a status code, not a length, so the buffer stays
    // zero-initialised and callers scan it to the terminator.
    out[0] = '\0';
    return g_cfgGet(g_cfgThread, key, out, size);
}

int settings_set(const char* key, const char* value)
{
    if (!g_cfgThread || !g_cfgSet || !key) return -1;
    return g_cfgSet(g_cfgThread, key, value ? value : "");
}

int settings_save()
{
    if (!g_cfgThread || !g_cfgSave) return -1;
    return g_cfgSave(g_cfgThread);
}

bool parse_lerf_file(const std::string& path, std::map<std::string, std::string>& out)
{
    out.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        // `<id>text<id>` is the LERF entry form; `key=value` is also accepted.
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

bool load_lerf_table(const char* lang, std::map<std::string, std::string>& out)
{
    out.clear();
    std::string l = (lang && std::strcmp(lang, "zh") == 0) ? "zh" : "en";
    std::string path = module_path(("resources\\cmd\\" + l + ".lerf").c_str());
    if (parse_lerf_file(path, out)) return true;
    if (l != "en") {
        std::string en = module_path("resources\\cmd\\en.lerf");
        parse_lerf_file(en, out);
    }
    return false;
}

const char* lerf_lookup(const std::map<std::string, std::string>& table, const char* id)
{
    static std::string s;
    if (!id) return "";
    auto it = table.find(id);
    s = (it != table.end()) ? it->second : std::string(id);
    return s.c_str();
}

} // namespace bm_runtime
