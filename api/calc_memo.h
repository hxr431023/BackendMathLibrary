// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_memo.h — "计算规律及答案"（calculation rules and answers）lookup table.
//
// The table stores expression -> answer pairs so a calculator can reuse a known
// result instead of evaluating it again. Two TMMF files back it:
//   resources\cmd\calc_memo.tmmf       baseline data, shipped with the source
//   resources\cmd\calc_memo.user.tmmf  what the running process learned, never committed
//
// Both are plain TMMF (`#MDRenderer Settings` header, `#` comments, key=value
// lines), so the existing bm_runtime parser and any text editor can read them.
// No absolute path is used: every file is resolved beside the module at runtime.

#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace bm_memo {

struct Entry
{
    std::string expr;      // normalized expression (the lookup key)
    std::string answer;    // result text reused on a hit
    long long   hits;      // how often this entry answered a query
    bool        runtime;   // true = learned at runtime, false = baseline data
};

// Load both tables. Safe to call repeatedly; returns the total entry count.
int init();

// Drop everything in memory and read both files again.
void reload();

// Total entry count (baseline + runtime).
size_t count();

// Lookup with an already normalized key. Returns nullptr on a miss.
const Entry* find(const std::string& normalizedExpr);

// Convenience wrapper: normalizes the input, then looks it up.
const Entry* lookup(const std::string& expr);

// Store an answer. Empty expr / answer is rejected. Returns 0 on success.
int store(const std::string& expr, const std::string& answer);

// Remove one entry (runtime first, then baseline shadow). Returns 0 when removed.
int forget(const std::string& expr);

// Write the runtime table back to calc_memo.user.tmmf. Returns 0 on success.
int save();

// Turn the raw text into the canonical key used by the table.
std::string normalize(const std::string& expr);

std::string baseline_path();
std::string runtime_path();

void set_auto_store(bool on);
bool auto_store();

// Called by the engine wrapper after an equals command completed: records the
// expression/result pair when auto-store is on. Never throws.
void record_after_equals(const wchar_t* expression, const wchar_t* result);

bool item(size_t idx, Entry& out);

// Empty the runtime table only; the baseline stays untouched.
void clear_runtime();

} // namespace bm_memo
