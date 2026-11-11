// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// native_runtime.h — the reusable half of the native layer.
//
// Everything here is independent of the calculator engine: locating the directory
// that holds the DLL, loading the GraalVM dependency DLLs from that directory, the
// TMMF settings store, the LERF language-file reader and UTF-8 conversions.
// calc_native.cpp keeps only the cm_* exports and the language state.
//
// Copy this pair of files into another project and it works unchanged: no path is
// hard-coded, every DLL is resolved beside the module at runtime.

#pragma once

#include <windows.h>
#include <map>
#include <string>

namespace bm_runtime {

// Directory of the module that contains this code (normally the folder holding
// CalcManager.dll). Computed once and cached.
const char* module_dir();

// module_dir() + "\\" + leaf.
std::string module_path(const char* leaf);

// Load a DLL from the module directory. Returns nullptr when the file is missing.
HMODULE load_beside(const char* name);

// ---- settings, persisted as TMMF by config.dll ----
//
// settings_init() creates the isolate, calls config_init and config_load_settings.
// All calls below become no-ops when config.dll is unavailable, so a missing
// dependency never fails the host process.

bool settings_init();
bool settings_ready();

int settings_get(const char* key, char* out, int size);
int settings_set(const char* key, const char* value);
int settings_save();

// ---- LERF language files ----

bool parse_lerf_file(const std::string& path, std::map<std::string, std::string>& out);

// Loads <module>/resources/cmd/<lang>.lerf, falling back to English.
// Returns true when the requested language itself was found.
bool load_lerf_table(const char* lang, std::map<std::string, std::string>& out);

// Entry text by id, or the id itself when the entry is missing.
const char* lerf_lookup(const std::map<std::string, std::string>& table, const char* id);

// ---- text ----

std::string utf8_from_wide(const wchar_t* w);
std::wstring wide_from_utf8(const char* s);

} // namespace bm_runtime
