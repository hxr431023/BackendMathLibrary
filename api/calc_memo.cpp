// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_memo.cpp — "计算规律及答案" lookup table backed by two TMMF files.
//
// Baseline data ships with the source (resources\cmd\calc_memo.tmmf); what the
// running process learns goes to resources\cmd\calc_memo.user.tmmf, which is
// machine state and stays out of the repository. A lookup checks the runtime
// table first (newest wins), then the baseline.
//
// Reused from native_runtime: module_path (no absolute path anywhere) and the
// UTF-8 conversions. The line parser is written here instead of calling
// parse_lerf_file so entry order is preserved, which the eviction and the
// enumeration API rely on.

#include "calc_memo.h"
#include "native_runtime.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <cwctype>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace {

const size_t kMaxRuntimeEntries = 4096;
const size_t kMaxAnswerLength   = 4096;

std::mutex                  g_mutex;
std::vector<bm_memo::Entry> g_runtime;
std::vector<bm_memo::Entry> g_base;
bool                        g_loaded = false;
bool                        g_autoStore = true;

inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

void trim(std::string& s)
{
    size_t b = 0, e = s.size();
    while (b < e && is_space(s[b])) ++b;
    while (e > b && is_space(s[e - 1])) --e;
    s = s.substr(b, e - b);
}

// One line of a TMMF table: `key=value`. `#` lines are comments, `meta.*` keys
// describe the file itself and are not entries.
bool parse_table(const std::string& path, std::vector<bm_memo::Entry>& out, bool runtime)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) return false;

    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos || eq == 0) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key.compare(0, 5, "meta.") == 0) continue;

        trim(key);
        trim(val);
        if (key.empty() || val.empty()) continue;

        bm_memo::Entry e;
        e.expr    = key;
        e.answer  = val;
        e.hits    = 0;
        e.runtime = runtime;
        out.push_back(e);
    }
    return true;
}

std::string timestamp_line()
{
    char buf[128] = {0};
    time_t now = time(nullptr);
    struct tm tmv;
    localtime_s(&tmv, &now);
    strftime(buf, sizeof(buf), "#%a %b %d %H:%M:%S %Y", &tmv);
    return buf;
}

void load_tables_locked()
{
    g_base.clear();
    g_runtime.clear();
    parse_table(bm_memo::baseline_path(), g_base, false);
    parse_table(bm_memo::runtime_path(), g_runtime, true);
    g_loaded = true;
}

// Runtime entries are scanned backwards: a later write shadows an earlier one.
int find_index_locked(const std::string& key, std::vector<bm_memo::Entry>& table)
{
    for (int i = static_cast<int>(table.size()) - 1; i >= 0; --i)
    {
        if (table[i].expr == key) return i;
    }
    return -1;
}

void copy_out(const std::string& s, char* out, int size)
{
    if (!out || size <= 0) return;
    size_t n = s.size();
    if (n > static_cast<size_t>(size - 1)) n = static_cast<size_t>(size - 1);
    memcpy(out, s.c_str(), n);
    out[n] = '\0';
}

} // namespace

namespace bm_memo {

std::string baseline_path()
{
    return bm_runtime::module_path("resources\\cmd\\calc_memo.tmmf");
}

std::string runtime_path()
{
    return bm_runtime::module_path("resources\\cmd\\calc_memo.user.tmmf");
}

int init()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_loaded) load_tables_locked();
    return static_cast<int>(g_base.size() + g_runtime.size());
}

void reload()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    load_tables_locked();
}

size_t count()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_loaded) load_tables_locked();
    return g_base.size() + g_runtime.size();
}

std::string normalize(const std::string& expr)
{
    std::wstring w = bm_runtime::wide_from_utf8(expr.c_str());
    std::wstring out;
    out.reserve(w.size());

    for (wchar_t c : w)
    {
        if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n' || c == 0x3000) continue;

        // Full-width digits and letters -> ASCII.
        if (c >= 0xFF10 && c <= 0xFF19) { out.push_back(static_cast<wchar_t>(L'0' + (c - 0xFF10))); continue; }
        if (c >= 0xFF21 && c <= 0xFF3A) { out.push_back(static_cast<wchar_t>(L'a' + (c - 0xFF21))); continue; }
        if (c >= 0xFF41 && c <= 0xFF5A) { out.push_back(static_cast<wchar_t>(L'a' + (c - 0xFF41))); continue; }

        switch (c)
        {
        case 0x00D7: out.push_back(L'*'); break;  // ×
        case 0x00F7: out.push_back(L'/'); break;  // ÷
        case 0x2212: out.push_back(L'-'); break;  // −
        case 0xFF0D: out.push_back(L'-'); break;  // －
        case 0xFF0B: out.push_back(L'+'); break;  // ＋
        case 0xFF0A: out.push_back(L'*'); break;  // ＊
        case 0xFF0F: out.push_back(L'/'); break;  // ／
        case 0xFF08: out.push_back(L'('); break;  // （
        case 0xFF09: out.push_back(L')'); break;  // ）
        case 0xFF1D: out.push_back(L'='); break;  // ＝
        case 0xFF0C: out.push_back(L','); break;  // ，
        case 0xFF0E: out.push_back(L'.'); break;  // ．
        case 0xFF3E: out.push_back(L'^'); break;  // ＾
        case 0xFF05: out.push_back(L'%'); break;  // ％
        case 0xFF01: out.push_back(L'!'); break;  // ！
        case 0xFF20: out.push_back(L'@'); break;  // ＠
        case 0x03C0: out.append(L"pi"); break;    // π
        case 0x2107: out.append(L"e"); break;     // ℇ
        default: out.push_back(static_cast<wchar_t>(towlower(c))); break;
        }
    }
    // The engine's expression token list may end with the equals sign; the key
    // never carries it, so the displayed "5+3=" and the stored "5+3" agree.
    while (!out.empty() && out.back() == L'=') out.pop_back();
    return bm_runtime::utf8_from_wide(out.c_str());
}

const Entry* find(const std::string& normalizedExpr)
{
    // The result is copied into a per-thread snapshot: the tables can grow on
    // another thread right after the lock is released, which would dangle.
    static thread_local Entry snapshot;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_loaded) load_tables_locked();

    int ri = find_index_locked(normalizedExpr, g_runtime);
    if (ri >= 0)
    {
        g_runtime[ri].hits++;
        snapshot = g_runtime[ri];
        return &snapshot;
    }
    int bi = find_index_locked(normalizedExpr, g_base);
    if (bi >= 0)
    {
        g_base[bi].hits++;
        snapshot = g_base[bi];
        return &snapshot;
    }
    return nullptr;
}

const Entry* lookup(const std::string& expr)
{
    return find(normalize(expr));
}

int store(const std::string& expr, const std::string& answer)
{
    std::string key = normalize(expr);
    std::string val = answer;
    trim(key);
    trim(val);
    if (key.empty() || val.empty()) return -1;
    if (val.size() > kMaxAnswerLength) val.resize(kMaxAnswerLength);

    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_loaded) load_tables_locked();

    int existing = find_index_locked(key, g_runtime);
    if (existing >= 0)
    {
        // Keep the newest text and move the entry to the end so it stays hot.
        g_runtime[existing].answer = val;
        Entry moved = g_runtime[existing];
        g_runtime.erase(g_runtime.begin() + existing);
        g_runtime.push_back(moved);
        return 0;
    }

    // A baseline entry already gives this answer: nothing to learn.
    int baseIdx = find_index_locked(key, g_base);
    if (baseIdx >= 0 && g_base[baseIdx].answer == val) return 0;

    if (g_runtime.size() >= kMaxRuntimeEntries)
        g_runtime.erase(g_runtime.begin());

    Entry e;
    e.expr    = key;
    e.answer  = val;
    e.hits    = 0;
    e.runtime = true;
    g_runtime.push_back(e);
    return 0;
}

int forget(const std::string& expr)
{
    std::string key = normalize(expr);
    if (key.empty()) return -1;

    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_loaded) load_tables_locked();

    int ri = find_index_locked(key, g_runtime);
    if (ri >= 0)
    {
        g_runtime.erase(g_runtime.begin() + ri);
        return 0;
    }
    int bi = find_index_locked(key, g_base);
    if (bi >= 0)
    {
        g_base.erase(g_base.begin() + bi);
        return 0;
    }
    return -1;
}

void clear_runtime()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_runtime.clear();
}

int save()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_loaded) load_tables_locked();

    // The directory holding the data is created by the build; if a host ships
    // only the DLL the save silently does nothing rather than failing.
    std::ofstream out(runtime_path().c_str(), std::ios::binary | std::ios::trunc);
    if (!out) return -1;

    std::string head = "#MDRenderer Settings\r\n";
    head += timestamp_line() + "\r\n";
    head += "# calc_memo.user.tmmf — results learned at runtime. Machine state, not source.\r\n";
    head += "# 运行时累积的常用结果，不属于源码。\r\n";
    head += "meta.version=1\r\n";
    head += "meta.kind=calculation-rules-and-answers\r\n";
    head += "meta.scope=runtime\r\n";
    head += "\r\n";
    out.write(head.c_str(), static_cast<std::streamsize>(head.size()));

    for (const Entry& e : g_runtime)
    {
        std::string line = e.expr + "=" + e.answer + "\r\n";
        out.write(line.c_str(), static_cast<std::streamsize>(line.size()));
    }
    out.flush();
    return out.good() ? 0 : -1;
}

void set_auto_store(bool on)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_autoStore = on;
}

bool auto_store()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_autoStore;
}

void record_after_equals(const wchar_t* expression, const wchar_t* result)
{
    if (!expression || !result) return;
    if (!auto_store()) return;

    std::string expr = bm_runtime::utf8_from_wide(expression);
    std::string res  = bm_runtime::utf8_from_wide(result);
    trim(expr);
    trim(res);
    // "5" = "5" carries nothing reusable: the engine echoes a plain value.
    if (expr.empty() || res.empty() || expr == res) return;

    store(expr, res);
}

bool item(size_t idx, Entry& out)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_loaded) load_tables_locked();
    if (idx < g_base.size()) { out = g_base[idx]; return true; }
    idx -= g_base.size();
    if (idx < g_runtime.size()) { out = g_runtime[idx]; return true; }
    return false;
}

} // namespace bm_memo

// ---------------------------------------------------------------------------
// C surface. Declared in calc_api.h; exported so any language can drive it.
// ---------------------------------------------------------------------------
extern "C" {

// Internal hook used by calc_engine.cpp after an equals command. Not part of the
// public header: hosts drive the table through cm_memo_store themselves.
__declspec(dllexport) void calc_memo_after_equals(const wchar_t* expression, const wchar_t* result)
{
    bm_memo::record_after_equals(expression, result);
}

__declspec(dllexport) int cm_memo_init()
{
    return bm_memo::init();
}
__declspec(dllexport) int cm_memo_reload()
{
    bm_memo::reload();
    return static_cast<int>(bm_memo::count());
}

__declspec(dllexport) int cm_memo_count()
{
    return static_cast<int>(bm_memo::count());
}

// Returns the reused answer as UTF-8, or nullptr when the expression is unknown.
__declspec(dllexport) const char* cm_memo_answer(const char* expr)
{
    static thread_local std::string buf;
    if (!expr) return nullptr;
    const bm_memo::Entry* e = bm_memo::lookup(expr);
    if (!e) return nullptr;
    buf = e->answer;
    return buf.c_str();
}

// 1 = hit (answer copied into out), 0 = miss.
__declspec(dllexport) int cm_memo_lookup(const char* expr, char* out, int size)
{
    if (!expr || !out || size <= 0) return 0;
    out[0] = '\0';
    const bm_memo::Entry* e = bm_memo::lookup(expr);
    if (!e) return 0;
    copy_out(e->answer, out, size);
    return 1;
}

__declspec(dllexport) int cm_memo_store(const char* expr, const char* answer)
{
    if (!expr || !answer) return -1;
    return bm_memo::store(expr, answer);
}

__declspec(dllexport) int cm_memo_forget(const char* expr)
{
    if (!expr) return -1;
    return bm_memo::forget(expr);
}

__declspec(dllexport) int cm_memo_save()
{
    return bm_memo::save();
}

__declspec(dllexport) int cm_memo_normalize(const char* expr, char* out, int size)
{
    if (!expr || !out || size <= 0) return -1;
    copy_out(bm_memo::normalize(expr), out, size);
    return 0;
}

// which: 0 = baseline file, 1 = runtime file.
__declspec(dllexport) const char* cm_memo_path(int which)
{
    static thread_local std::string buf;
    buf = (which == 1) ? bm_memo::runtime_path() : bm_memo::baseline_path();
    return buf.c_str();
}

__declspec(dllexport) int cm_memo_item(int idx, char* exprOut, int exprSize,
                                       char* answerOut, int answerSize,
                                       int* hits, int* runtime)
{
    if (idx < 0) return 0;
    bm_memo::Entry e;
    if (!bm_memo::item(static_cast<size_t>(idx), e)) return 0;
    copy_out(e.expr, exprOut, exprSize);
    copy_out(e.answer, answerOut, answerSize);
    if (hits) *hits = static_cast<int>(e.hits);
    if (runtime) *runtime = e.runtime ? 1 : 0;
    return 1;
}

__declspec(dllexport) void cm_memo_clear_runtime()
{
    bm_memo::clear_runtime();
}

__declspec(dllexport) void cm_memo_set_auto_store(int on)
{
    bm_memo::set_auto_store(on != 0);
}

__declspec(dllexport) int cm_memo_get_auto_store()
{
    return bm_memo::auto_store() ? 1 : 0;
}

// Hit counter for one entry, so a host can show how much work the table saved.
// find() already holds the table lock, so this does not take it again.
__declspec(dllexport) int cm_memo_hits(const char* expr)
{
    if (!expr) return -1;
    const bm_memo::Entry* e = bm_memo::find(bm_memo::normalize(expr));
    return e ? static_cast<int>(e->hits) : -1;
}

} // extern "C"
