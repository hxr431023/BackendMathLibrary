// calc_cmp.cpp — Comparison zone (pure C, array-in / array-out).
//
// Input and output are both lists (arrays) of wide strings.
//   * Plain compare   : inputs = { "1.5", "2" , ... }  -> compare numbers.
//   * Unit compare    : inputs = { "1 kg", "500 g", ... } -> unify all quantities to the
//                         first input's unit (via the unit converter), strip units, compare.
// Each output item is a "key=value" string, usable both as an array and as a dictionary.
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>

// Internal declarations of the unit-converter exports (same DLL, direct calls).
extern "C"
{
    __declspec(dllexport) void* cm_unit_new(void);
    __declspec(dllexport) void cm_unit_free(void* h);
    __declspec(dllexport) int cm_unit_category_count(void* h);
    __declspec(dllexport) const wchar_t* cm_unit_category_name(void* h, int idx);
    __declspec(dllexport) void cm_unit_set_category(void* h, int idx);
    __declspec(dllexport) int cm_unit_unit_count(void* h);
    __declspec(dllexport) const wchar_t* cm_unit_unit_abbrev(void* h, int idx);
    __declspec(dllexport) void cm_unit_set_units(void* h, int fromIdx, int toIdx);
    __declspec(dllexport) void cm_unit_switch_active(void* h, const wchar_t* value);
    __declspec(dllexport) const wchar_t* cm_unit_convert(void* h, int catIdx, int fromIdx, int toIdx, const wchar_t* value);
}

using namespace std;

namespace
{
    struct CMCompare
    {
        vector<wstring> items; // each item: "key=value"
    };

    struct UInfo { int cat; int unit; wstring abbr; };
    struct Parsed { int cat; int unit; wstring abbr; wstring valueStr; double value; };

    wstring trim(const wstring& s)
    {
        size_t b = s.find_first_not_of(L" \t");
        if (b == wstring::npos) return L"";
        size_t e = s.find_last_not_of(L" \t");
        return s.substr(b, e - b + 1);
    }

    bool parseNumber(const wstring& s, double& out)
    {
        if (s.empty()) return false;
        wchar_t* end = nullptr;
        double v = wcstod(s.c_str(), &end);
        if (end == s.c_str()) return false;   // no digits parsed
        // allow trailing spaces only (we already trimmed)
        while (*end == L' ' || *end == L'\t') ++end;
        if (*end != L'\0') return false;
        out = v;
        return true;
    }

    wstring fmt(double d)
    {
        wchar_t buf[64];
        swprintf(buf, 64, L"%.12g", d);
        return wstring(buf);
    }

    // Parse "1 kg" / "500g" / "100°C" into a (category, unit, value). Longest unit match wins.
    bool parseInput(const wchar_t* input, const vector<UInfo>& all, Parsed& out)
    {
        wstring s = input ? input : L"";
        size_t bestLen = 0;
        int best = -1;
        for (int k = 0; k < (int)all.size(); ++k)
        {
            const wstring& ab = all[k].abbr;
            size_t l = ab.size();
            if (l < 1 || l > s.size()) continue;
            if (s.compare(s.size() - l, l, ab) != 0) continue; // not a suffix
            if (l <= bestLen) continue;                          // prefer longest
            wstring np = trim(s.substr(0, s.size() - l));
            double dv = 0.0;
            if (!parseNumber(np, dv)) continue;
            bestLen = l; best = k; out.cat = all[k].cat; out.unit = all[k].unit;
            out.abbr = all[k].abbr; out.valueStr = np; out.value = dv;
        }
        return best >= 0;
    }

    void fixItems(CMCompare* c)
    {
        // items are never modified after this, so their c_str() pointers stay valid.
        (void)c;
    }

    void buildStats(vector<double>& v, CMCompare* c)
    {
        int n = (int)v.size();
        double mn = v[0], mx = v[0], sum = 0.0;
        for (int i = 0; i < n; ++i) { if (v[i] < mn) mn = v[i]; if (v[i] > mx) mx = v[i]; sum += v[i]; }
        c->items.push_back(L"count=" + to_wstring(n));
        c->items.push_back(L"min=" + fmt(mn));
        c->items.push_back(L"max=" + fmt(mx));
        c->items.push_back(L"sum=" + fmt(sum));
        c->items.push_back(L"mean=" + fmt(sum / n));
        double a = v[0], b = v[1];
        int rel = (a > b + 1e-9) ? 1 : ((a < b - 1e-9) ? -1 : 0);
        c->items.push_back(L"result=" + wstring(rel > 0 ? L"greater" : (rel < 0 ? L"less" : L"equal")));
        c->items.push_back(L"equal=" + to_wstring(rel == 0 ? 1 : 0));
        c->items.push_back(L"diff=" + fmt(a - b));
        c->items.push_back(L"abs_diff=" + fmt(fabs(a - b)));
        if (fabs(b) > 1e-15)
        {
            c->items.push_back(L"ratio=" + fmt(a / b));
            c->items.push_back(L"percent=" + fmt(100.0 * a / b) + L"%");
        }
    }
}

extern "C"
{
    __declspec(dllexport) void* cm_cmp_new(void)
    {
        try { return new CMCompare; } catch (...) { return nullptr; }
    }

    __declspec(dllexport) void cm_cmp_free(void* h)
    {
        delete static_cast<CMCompare*>(h);
    }

    // Plain-number compare. inputs = { a, b, ... }.
    __declspec(dllexport) int cm_cmp_compare(void* h, const wchar_t** inputs, int n)
    {
        CMCompare* c = static_cast<CMCompare*>(h);
        if (!c || !inputs || n < 2) return 0;
        try
        {
            c->items.clear();
            vector<double> v(n);
            for (int i = 0; i < n; ++i)
            {
                wchar_t* end = nullptr;
                double dv = wcstod(inputs[i] ? inputs[i] : L"", &end);
                if (end == (inputs[i] ? inputs[i] : L"")) return 0; // not a number
                v[i] = dv;
            }
            c->items.push_back(L"a=" + wstring(inputs[0]));
            c->items.push_back(L"b=" + wstring(inputs[1]));
            buildStats(v, c);
            fixItems(c);
            return (int)c->items.size();
        }
        catch (...) { return 0; }
    }

    // Unit compare. inputs = { "1 kg", "500 g", ... }. All unified to the first input's unit.
    __declspec(dllexport) int cm_cmp_compare_units(void* h, const wchar_t** inputs, int n)
    {
        CMCompare* c = static_cast<CMCompare*>(h);
        if (!c || !inputs || n < 2) return 0;
        void* u = cm_unit_new();
        if (!u) return 0;
        try
        {
            // Enumerate all categories and units.
            vector<UInfo> all;
            int cc = cm_unit_category_count(u);
            for (int cat = 0; cat < cc; ++cat)
            {
                cm_unit_set_category(u, cat);
                int uc = cm_unit_unit_count(u);
                for (int ui = 0; ui < uc; ++ui)
                {
                    const wchar_t* ab = cm_unit_unit_abbrev(u, ui);
                    if (ab) all.push_back(UInfo{ cat, ui, wstring(ab) });
                }
            }

            vector<Parsed> p(n);
            for (int i = 0; i < n; ++i)
                if (!parseInput(inputs[i], all, p[i])) { cm_unit_free(u); return 0; }

            for (int i = 1; i < n; ++i)
                if (p[i].cat != p[0].cat) { cm_unit_free(u); return 0; } // different categories

            // Convert every other input into the first input's unit.
            vector<double> v(n);
            for (int i = 0; i < n; ++i)
            {
                double val = (i == 0) ? p[0].value : 0.0;
                if (i != 0)
                {
                    const wchar_t* conv = cm_unit_convert(u, p[0].cat, p[i].unit, p[0].unit, p[i].valueStr.c_str());
                    val = conv ? wcstod(conv, nullptr) : 0.0;
                }
                if (fabs(val) < 1e-9) val = 0.0; // wipe floating-point noise near zero
                v[i] = val;
            }

            c->items.clear();
            c->items.push_back(L"category=" + wstring(cm_unit_category_name(u, p[0].cat)));
            c->items.push_back(L"common_unit=" + p[0].abbr);
            c->items.push_back(L"value0=" + fmt(v[0]));
            for (int i = 1; i < n; ++i)
                c->items.push_back(L"value" + to_wstring(i) + L"=" + fmt(v[i]));
            buildStats(v, c);
            cm_unit_free(u);
            fixItems(c);
            return (int)c->items.size();
        }
        catch (...)
        {
            cm_unit_free(u);
            return 0;
        }
    }

    __declspec(dllexport) const wchar_t* cm_cmp_item(void* h, int idx)
    {
        CMCompare* c = static_cast<CMCompare*>(h);
        if (!c || idx < 0 || idx >= (int)c->items.size()) return L"";
        return c->items[idx].c_str();
    }
}
