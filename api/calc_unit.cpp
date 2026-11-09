// 本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）。
// calc_unit.cpp — Unit converter C wrapper over the original UnitConverter engine.
// The original unit data (UnitConverterDataConstants.h) is a generated artifact not present in
// the repo, so this file supplies a built-in set of common units. Each unit is modelled linearly
// as value_k = a*value_u + b against a hidden per-category base, which fits UnitConverter's
// Convert(v, {ratio, offset, offsetFirst}) exactly (offsetFirst=false).
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cmath>
#include <cstdlib>
#include <cwchar>
#include <specstrings.h>

#include "UnitConverter.h"

using namespace std;
using namespace UnitConversionManager;

// ---------------------------------------------------------------------------
// Built-in unit data.
// ---------------------------------------------------------------------------
namespace
{
    struct UDef { int id; const wchar_t* name; const wchar_t* abbrev; double a; double b; };
    struct CatDef { int id; const wchar_t* name; bool neg; vector<UDef> units; };

    const vector<CatDef>& cats()
    {
        static const vector<CatDef> c = {
            // Length (base: metre)
            { 1, L"Length", true, {
                { 101, L"Meter", L"m", 1.0, 0.0 }, { 102, L"Kilometer", L"km", 1000.0, 0.0 },
                { 103, L"Centimeter", L"cm", 0.01, 0.0 }, { 104, L"Millimeter", L"mm", 0.001, 0.0 },
                { 105, L"Mile", L"mi", 1609.344, 0.0 }, { 106, L"Foot", L"ft", 0.3048, 0.0 },
                { 107, L"Inch", L"in", 0.0254, 0.0 }, { 108, L"Yard", L"yd", 0.9144, 0.0 },
            }},
            // Mass (base: kilogram)
            { 2, L"Mass", true, {
                { 201, L"Kilogram", L"kg", 1.0, 0.0 }, { 202, L"Gram", L"g", 0.001, 0.0 },
                { 203, L"Milligram", L"mg", 1e-6, 0.0 }, { 204, L"Metric ton", L"t", 1000.0, 0.0 },
                { 205, L"Pound", L"lb", 0.45359237, 0.0 }, { 206, L"Ounce", L"oz", 0.0283495231, 0.0 },
            }},
            // Temperature (base: kelvin): C a=1,b=273.15 ; F a=5/9,b=255.3722...
            { 3, L"Temperature", true, {
                { 301, L"Celsius", L"\u00B0C", 1.0, 273.15 },
                { 302, L"Fahrenheit", L"\u00B0F", 5.0 / 9.0, 255.37222222222222 },
                { 303, L"Kelvin", L"K", 1.0, 0.0 },
            }},
            // Speed (base: metre/second)
            { 4, L"Speed", true, {
                { 401, L"Metre/second", L"m/s", 1.0, 0.0 }, { 402, L"Kilometre/hour", L"km/h", 1000.0 / 3600.0, 0.0 },
                { 403, L"Mile/hour", L"mph", 0.44704, 0.0 }, { 404, L"Knot", L"knot", 0.5144444, 0.0 },
            }},
            // Area (base: square metre)
            { 5, L"Area", false, {
                { 501, L"Square metre", L"m\u00B2", 1.0, 0.0 }, { 502, L"Square kilometre", L"km\u00B2", 1e6, 0.0 },
                { 503, L"Hectare", L"ha", 10000.0, 0.0 }, { 504, L"Acre", L"ac", 4046.8564224, 0.0 },
                { 505, L"Square foot", L"ft\u00B2", 0.09290304, 0.0 },
            }},
            // Volume (base: litre)
            { 6, L"Volume", false, {
                { 601, L"Litre", L"L", 1.0, 0.0 }, { 602, L"Millilitre", L"mL", 0.001, 0.0 },
                { 603, L"Cubic metre", L"m\u00B3", 1000.0, 0.0 }, { 604, L"US gallon", L"gal", 3.785411784, 0.0 },
                { 605, L"Cubic foot", L"ft\u00B3", 28.316846592, 0.0 },
            }},
            // Time (base: second)
            { 7, L"Time", true, {
                { 701, L"Second", L"s", 1.0, 0.0 }, { 702, L"Minute", L"min", 60.0, 0.0 },
                { 703, L"Hour", L"h", 3600.0, 0.0 }, { 704, L"Day", L"d", 86400.0, 0.0 },
                { 705, L"Week", L"wk", 604800.0, 0.0 }, { 706, L"Year", L"y", 31557600.0, 0.0 },
            }},
            // Data (base: byte, binary)
            { 8, L"Data", false, {
                { 801, L"Byte", L"B", 1.0, 0.0 }, { 802, L"Kilobyte", L"KB", 1024.0, 0.0 },
                { 803, L"Megabyte", L"MB", 1048576.0, 0.0 }, { 804, L"Gigabyte", L"GB", 1073741824.0, 0.0 },
                { 805, L"Terabyte", L"TB", 1099511627776.0, 0.0 },
            }},
        };
        return c;
    }

    const UDef* find_unit(int id)
    {
        for (auto& cat : cats())
            for (auto& u : cat.units)
                if (u.id == id)
                    return &u;
        return nullptr;
    }
}

// ---------------------------------------------------------------------------
// Data loader.
// ---------------------------------------------------------------------------
class CUnitConverterDataLoader : public IConverterDataLoader
{
public:
    void LoadData() override {}

    vector<Category> GetOrderedCategories() override
    {
        vector<Category> out;
        for (auto& cat : cats())
            out.emplace_back(cat.id, cat.name, cat.neg);
        return out;
    }

    vector<Unit> GetOrderedUnits(const Category& c) override
    {
        vector<Unit> out;
        for (auto& cat : cats())
            if (cat.id == c.id)
                for (auto& u : cat.units)
                    out.emplace_back(u.id, u.name, u.abbrev, true, true, false);
        return out;
    }

    unordered_map<Unit, ConversionData, UnitHash> LoadOrderedRatios(const Unit& u) override
    {
        unordered_map<Unit, ConversionData, UnitHash> out;
        const UDef* from = find_unit(u.id);
        if (!from)
            return out;
        for (auto& cat : cats())
            for (auto& v : cat.units)
                if (v.id != u.id)
                    out.emplace(Unit{ v.id, v.name, v.abbrev, true, true, false },
                                ConversionData{ from->a / v.a, (from->b - v.b) / v.a, false });
        return out;
    }

    bool SupportsCategory(const Category&) override { return true; }
};

// ---------------------------------------------------------------------------
// Display callback: cache from/to values.
// ---------------------------------------------------------------------------
class CUnitConverterVMCallback : public IUnitConverterVMCallback
{
public:
    wstring fromValue;
    wstring toValue;

    void DisplayCallback(const wstring& from, const wstring& to) override
    {
        fromValue = from;
        toValue = to;
    }
    void SuggestedValueCallback(const vector<tuple<wstring, Unit>>&) override {}
    void MaxDigitsReached() override {}
};

// ---------------------------------------------------------------------------
// Handle.
// ---------------------------------------------------------------------------
struct CMUnit
{
    shared_ptr<UnitConverter> conv;
    shared_ptr<CUnitConverterDataLoader> loader;
    shared_ptr<CUnitConverterVMCallback> cb;
    wstring lastString; // stable buffer for returned const wchar_t* names
};

extern "C"
{
    __declspec(dllexport) void* cm_unit_new(void)
    {
        try
        {
            CMUnit* m = new CMUnit;
            m->loader = make_shared<CUnitConverterDataLoader>();
            m->cb = make_shared<CUnitConverterVMCallback>();
            m->conv = make_shared<UnitConverter>(m->loader);
            m->conv->SetViewModelCallback(m->cb);
            m->conv->Initialize();
            return m;
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) void cm_unit_free(void* h)
    {
        delete static_cast<CMUnit*>(h);
    }

    // --- categories ---
    __declspec(dllexport) int cm_unit_category_count(void* h)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        return (m && m->conv) ? static_cast<int>(m->conv->GetCategories().size()) : 0;
    }
    __declspec(dllexport) const wchar_t* cm_unit_category_name(void* h, int idx)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        if (!m || !m->conv) return L"";
        try
        {
            auto catsv = m->conv->GetCategories();
            if (idx >= 0 && idx < (int)catsv.size())
            {
                m->lastString = catsv[idx].name;
                return m->lastString.c_str();
            }
        }
        catch (...) {}
        return L"";
    }
    __declspec(dllexport) void cm_unit_set_category(void* h, int idx)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        if (!m || !m->conv) return;
        try
        {
            auto catsv = m->conv->GetCategories();
            if (idx >= 0 && idx < (int)catsv.size())
                m->conv->SetCurrentCategory(catsv[idx]);
        }
        catch (...) {}
    }
    __declspec(dllexport) int cm_unit_current_category_id(void* h)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        return (m && m->conv) ? m->conv->GetCurrentCategory().id : -1;
    }

    // --- units within current category ---
    __declspec(dllexport) int cm_unit_unit_count(void* h)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        if (!m || !m->conv) return 0;
        try { return static_cast<int>(m->loader->GetOrderedUnits(m->conv->GetCurrentCategory()).size()); }
        catch (...) { return 0; }
    }
    __declspec(dllexport) const wchar_t* cm_unit_unit_name(void* h, int idx)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        if (!m || !m->conv) return L"";
        try
        {
            auto us = m->loader->GetOrderedUnits(m->conv->GetCurrentCategory());
            if (idx >= 0 && idx < (int)us.size())
            {
                m->lastString = us[idx].name;
                return m->lastString.c_str();
            }
        }
        catch (...) {}
        return L"";
    }
    __declspec(dllexport) const wchar_t* cm_unit_unit_abbrev(void* h, int idx)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        if (!m || !m->conv) return L"";
        try
        {
            auto us = m->loader->GetOrderedUnits(m->conv->GetCurrentCategory());
            if (idx >= 0 && idx < (int)us.size())
            {
                m->lastString = us[idx].abbreviation;
                return m->lastString.c_str();
            }
        }
        catch (...) {}
        return L"";
    }

    // One-step conversion: value (in from unit) -> value in to unit, without touching
    // the interactive from/to state. Linear model: value_base = a*value + b.
    // from -> to: ratio = a_from/a_to, offset = (b_from-b_to)/a_to, out = v*ratio + offset.
    __declspec(dllexport) const wchar_t* cm_unit_convert(void* h, int catIdx, int fromIdx, int toIdx, const wchar_t* value)
    {
        CMUnit* m = static_cast<CMUnit*>(h);
        if (!m || !value) return L"";
        try
        {
            const auto& cl = cats();
            if (catIdx < 0 || catIdx >= (int)cl.size()) return L"";
            const auto& cat = cl[catIdx];
            if (fromIdx < 0 || fromIdx >= (int)cat.units.size() || toIdx < 0 || toIdx >= (int)cat.units.size()) return L"";
            const auto& fu = cat.units[fromIdx];
            const auto& tu = cat.units[toIdx];
            wchar_t* end = nullptr;
            double v = wcstod(value, &end);
            if (end == value) return L"";            // no digits parsed
            while (*end == L' ' || *end == L'\t') ++end;
            if (*end != L'\0') return L"";           // trailing junk after number
            double out = v * (fu.a / tu.a) + ((fu.b - tu.b) / tu.a);
            wchar_t buf[64];
            swprintf(buf, 64, L"%.12g", out);
            m->lastString = buf;
            return m->lastString.c_str();
        }
        catch (...) { return L""; }
    }
}
