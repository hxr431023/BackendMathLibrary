// calc_engine.cpp — Full-engine C wrapper over the original CalculatorManager (CEngine command model).
// Implements the ICalcDisplay + IResourceProvider callbacks and exposes every public
// CalculatorManager API as a plain extern "C" function. Consumers pass an engine handle.
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <array>
#include <unordered_map>
#include <cwchar>
#include <specstrings.h>

#include "CalculatorManager.h"
#include "CalculatorResource.h"
#include "CalculatorHistory.h"
#include "Header Files/RadixType.h"

using namespace std;
using namespace CalculationManager;

// ---------------------------------------------------------------------------
// Resource text: operator symbols, digit labels, function names, error messages.
// ---------------------------------------------------------------------------
static const wchar_t* res_num(int i)
{
    static const wchar_t* t[121] = {
        /*0*/  L"\u00B1", /*1*/ L"CE",     /*2*/ L"CE",     /*3*/ L"\u232B", /*4*/ L".",
        /*5*/  L"",       /*6*/ L"AND",    /*7*/ L"OR",     /*8*/ L"XOR",    /*9*/ L"LSH",
        /*10*/ L"RSH",    /*11*/ L"\u00F7",/*12*/ L"\u00D7",/*13*/ L"+",     /*14*/ L"-",
        /*15*/ L"mod",    /*16*/ L"\u02B8\u221Ax", /*17*/ L"^", /*18*/ L"Int", /*19*/ L"ROL",
        /*20*/ L"ROR",    /*21*/ L"NOT",   /*22*/ L"sin",   /*23*/ L"cos",   /*24*/ L"tan",
        /*25*/ L"sinh",   /*26*/ L"cosh",  /*27*/ L"tanh",  /*28*/ L"ln",    /*29*/ L"log",
        /*30*/ L"\u221A", /*31*/ L"x\u00B2",/*32*/ L"x\u00B3",/*33*/ L"!",   /*34*/ L"1/x",
        /*35*/ L"dms",    /*36*/ L"",      /*37*/ L"10^x",  /*38*/ L"%",     /*39*/ L"SCI",
        /*40*/ L"\u03C0", /*41*/ L"=",     /*42*/ L"MC",    /*43*/ L"MR",    /*44*/ L"MS",
        /*45*/ L"M+",     /*46*/ L"M-",    /*47*/ L"exp",   /*48*/ L"(",     /*49*/ L")",
        /*50*/ L"0", /*51*/ L"1", /*52*/ L"2", /*53*/ L"3", /*54*/ L"4",
        /*55*/ L"5", /*56*/ L"6", /*57*/ L"7", /*58*/ L"8", /*59*/ L"9",
        /*60*/ L"A", /*61*/ L"B", /*62*/ L"C", /*63*/ L"D", /*64*/ L"E", /*65*/ L"F",
        /*66*/ L"frac",  /*67*/ L"sind",  /*68*/ L"cosd",  /*69*/ L"tand",
        /*70*/ L"asind", /*71*/ L"acosd", /*72*/ L"atand", /*73*/ L"sinr",
        /*74*/ L"cosr",  /*75*/ L"tanr",  /*76*/ L"asinr", /*77*/ L"acosr",
        /*78*/ L"atanr", /*79*/ L"sing",  /*80*/ L"cosg",  /*81*/ L"tang",
        /*82*/ L"asing", /*83*/ L"acosg", /*84*/ L"atang", /*85*/ L"asinh",
        /*86*/ L"acosh", /*87*/ L"atanh", /*88*/ L"e^x",   /*89*/ L"10^x",
        /*90*/ L"\u221A",/*91*/ L"x\u00B2",/*92*/ L"x\u00B3",/*93*/ L"\u221B",/*94*/ L"!",
        /*95*/ L"1/x",   /*96*/ L"deg",   /*97*/ L"\u00B1",/*98*/ L"RSH",
        /*99*/ L"Cannot divide by zero",        /*100*/ L"Invalid input",
        /*101*/ L"Undefined",                   /*102*/ L"Positive infinity",
        /*103*/ L"Negative infinity",           /*104*/ L"Aborted",
        /*105*/ L"Not enough memory",           /*106*/ L"Too many digits",
        /*107*/ L"Overflow",                    /*108*/ L"No result",
        /*109*/ L"Insufficient data",           /*110*/ L"",
        /*111*/ L"Unknown character",           /*112*/ L"Unknown function",
        /*113*/ L"Unexpected number",           /*114*/ L"Unexpected character",
        /*115*/ L"Unexpected size",             /*116*/ L"Mismatched close parenthesis",
        /*117*/ L"Unexpected end",              /*118*/ L"Sign-invalid error",
        /*119*/ L"Input overflow",              /*120*/ L"Output overflow",
    };
    return (i >= 0 && i <= 120) ? t[i] : L"";
}

static wstring res_named(wstring_view id)
{
    static const unordered_map<wstring, wstring> m = {
        { L"SecDeg", L"sec" }, { L"SecRad", L"sec" }, { L"SecGrad", L"sec" },
        { L"InverseSecDeg", L"arcsec" }, { L"InverseSecRad", L"arcsec" }, { L"InverseSecGrad", L"arcsec" },
        { L"CscDeg", L"csc" }, { L"CscRad", L"csc" }, { L"CscGrad", L"csc" },
        { L"InverseCscDeg", L"arccsc" }, { L"InverseCscRad", L"arccsc" }, { L"InverseCscGrad", L"arccsc" },
        { L"CotDeg", L"cot" }, { L"CotRad", L"cot" }, { L"CotGrad", L"cot" },
        { L"InverseCotDeg", L"arccot" }, { L"InverseCotRad", L"arccot" }, { L"InverseCotGrad", L"arccot" },
        { L"Sech", L"sech" }, { L"InverseSech", L"arcsech" },
        { L"Csch", L"csch" }, { L"InverseCsch", L"arccsch" },
        { L"Coth", L"coth" }, { L"InverseCoth", L"arccoth" },
        { L"TwoPowX", L"2^x" }, { L"LogBaseY", L"log\u2093y" },
        { L"Abs", L"abs" }, { L"Floor", L"floor" }, { L"Ceil", L"ceil" },
        { L"Nand", L"NAND" }, { L"Nor", L"NOR" }, { L"CubeRoot", L"\u221B" },
        { L"ProgrammerMod", L"mod" },
        { L"sDecimal", L"." }, { L"sThousand", L"," }, { L"sGrouping", L"3;0" },
    };
    auto it = m.find(wstring(id));
    return it == m.end() ? wstring() : it->second;
}

class CResourceProvider : public IResourceProvider
{
public:
    wstring GetCEngineString(wstring_view id) override
    {
        // Numeric ids: "0".."120"
        int n = 0;
        bool numeric = true;
        for (wchar_t c : id)
        {
            if (c < L'0' || c > L'9') { numeric = false; break; }
            n = n * 10 + (c - L'0');
            if (n > 120) { numeric = false; break; }
        }
        if (numeric)
        {
            const wchar_t* t = res_num(n);
            return t ? wstring(t) : wstring();
        }
        return res_named(id);
    }
};

// ---------------------------------------------------------------------------
// Display sink: caches engine output so the C layer can query it.
// ---------------------------------------------------------------------------
class CDisplaySink : public ICalcDisplay
{
public:
    wstring display;
    bool isError = false;
    wstring expression;
    unsigned int parenthesis = 0;
    vector<wstring> memory;
    bool inputChanged = false;
    bool maxDigits = false;
    bool binaryOp = false;

    void SetPrimaryDisplay(const wstring& text, bool err) override { display = text; isError = err; }
    void SetIsInError(bool err) override { isError = err; }
    void SetExpressionDisplay(
        shared_ptr<vector<pair<wstring, int>>> const& tokens,
        shared_ptr<vector<shared_ptr<IExpressionCommand>>> const&) override
    {
        expression.clear();
        if (tokens)
        {
            for (auto& t : *tokens)
                expression += t.first;
        }
    }
    void SetParenthesisNumber(unsigned int c) override { parenthesis = c; }
    void OnNoRightParenAdded() override {}
    void MaxDigitsReached() override { maxDigits = true; }
    void BinaryOperatorReceived() override { binaryOp = true; }
    void OnHistoryItemAdded(unsigned int) override {}
    void SetMemorizedNumbers(const vector<wstring>& mn) override { memory = mn; }
    void MemoryItemChanged(unsigned int) override {}
    void InputChanged() override { inputChanged = true; }
};

// ---------------------------------------------------------------------------
// Engine handle.
// ---------------------------------------------------------------------------
struct CMEngine
{
    CalculatorManager* mgr = nullptr;
    CDisplaySink* display = nullptr;
    CResourceProvider* res = nullptr;
};

extern "C"
{
    __declspec(dllexport) void* cm_engine_new(void)
    {
        try
        {
            CMEngine* e = new CMEngine;
            e->res = new CResourceProvider;
            e->display = new CDisplaySink;
            e->mgr = new CalculatorManager(e->display, e->res);
            e->mgr->SetScientificMode(); // default: scientific (operator precedence) engine
            return e;
        }
        catch (...) { return nullptr; }
    }

    __declspec(dllexport) void cm_engine_free(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (!e) return;
        delete e->mgr;
        delete e->display;
        delete e->res;
        delete e;
    }

    __declspec(dllexport) void cm_engine_reset(void* h, int clearMemory)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (!e || !e->mgr) return;
        try { e->mgr->Reset(clearMemory != 0); } catch (...) {}
    }

    __declspec(dllexport) void cm_engine_send_command(void* h, int command)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (!e || !e->mgr) return;
        try { e->mgr->SendCommand(static_cast<Command>(command)); } catch (...) {}
    }

    __declspec(dllexport) void cm_engine_set_scientific_mode(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->SetScientificMode(); } catch (...) {} }
    }
    __declspec(dllexport) void cm_engine_set_programmer_mode(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->SetProgrammerMode(); } catch (...) {} }
    }

    // --- state queries ---
    __declspec(dllexport) const wchar_t* cm_engine_get_display(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->display) ? e->display->display.c_str() : L"";
    }
    __declspec(dllexport) int cm_engine_is_error(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->display && e->display->isError) ? 1 : 0;
    }
    __declspec(dllexport) const wchar_t* cm_engine_get_expression(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->display) ? e->display->expression.c_str() : L"";
    }
    __declspec(dllexport) unsigned int cm_engine_parenthesis_count(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->display) ? e->display->parenthesis : 0;
    }
    __declspec(dllexport) int cm_engine_is_input_empty(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->mgr) ? (e->mgr->IsInputEmpty() ? 1 : 0) : 0;
    }
    __declspec(dllexport) int cm_engine_is_recording(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->mgr) ? (e->mgr->IsEngineRecording() ? 1 : 0) : 0;
    }
    __declspec(dllexport) int cm_engine_degree_mode(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->mgr) ? static_cast<int>(e->mgr->GetCurrentDegreeMode()) : 0;
    }
    __declspec(dllexport) int cm_engine_input_changed(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (!e || !e->display) return 0;
        int v = e->display->inputChanged ? 1 : 0;
        e->display->inputChanged = false;
        return v;
    }

    // --- radix / precision ---
    __declspec(dllexport) void cm_engine_set_radix(void* h, int radix)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->SetRadix(static_cast<RadixType>(radix)); } catch (...) {} }
    }
    __declspec(dllexport) wchar_t* cm_engine_get_result_for_radix(void* h, unsigned int radix, int precision, int groupDigits)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (!e || !e->mgr) return nullptr;
        wstring s;
        try { s = e->mgr->GetResultForRadix(radix, precision, groupDigits != 0); }
        catch (...) { return nullptr; }
        wchar_t* out = new wchar_t[s.size() + 1];
        wcscpy_s(out, s.size() + 1, s.c_str());
        return out;
    }
    __declspec(dllexport) void cm_engine_set_precision(void* h, int precision)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->SetPrecision(precision); } catch (...) {} }
    }
    __declspec(dllexport) void cm_engine_update_max_int_digits(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->UpdateMaxIntDigits(); } catch (...) {} }
    }
    __declspec(dllexport) wchar_t cm_engine_decimal_separator(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->mgr) ? e->mgr->DecimalSeparator() : L'.';
    }

    // --- memory ---
    __declspec(dllexport) void cm_engine_memorize(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->MemorizeNumber(); } catch (...) {} }
    }
    __declspec(dllexport) void cm_engine_memory_load(void* h, unsigned int i)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->MemorizedNumberLoad(i); } catch (...) {} }
    }
    __declspec(dllexport) void cm_engine_memory_add(void* h, unsigned int i)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->MemorizedNumberAdd(i); } catch (...) {} }
    }
    __declspec(dllexport) void cm_engine_memory_subtract(void* h, unsigned int i)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->MemorizedNumberSubtract(i); } catch (...) {} }
    }
    __declspec(dllexport) void cm_engine_memory_clear(void* h, unsigned int i)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->MemorizedNumberClear(i); } catch (...) {} }
    }
    __declspec(dllexport) void cm_engine_memory_clear_all(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->MemorizedNumberClearAll(); } catch (...) {} }
    }
    __declspec(dllexport) int cm_engine_memory_count(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->display) ? static_cast<int>(e->display->memory.size()) : 0;
    }
    __declspec(dllexport) const wchar_t* cm_engine_memory_get(void* h, unsigned int i)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (!e || !e->display || i >= e->display->memory.size()) return L"";
        return e->display->memory[i].c_str();
    }

    // --- history ---
    __declspec(dllexport) int cm_engine_history_count(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->mgr) ? static_cast<int>(e->mgr->GetHistoryItems().size()) : 0;
    }
    __declspec(dllexport) wchar_t* cm_engine_history_item(void* h, unsigned int idx)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (!e || !e->mgr) return nullptr;
        auto& items = e->mgr->GetHistoryItems();
        if (idx >= items.size()) return nullptr;
        wstring s = items[idx]->historyItemVector.expression + L" = " + items[idx]->historyItemVector.result;
        wchar_t* out = new wchar_t[s.size() + 1];
        wcscpy_s(out, s.size() + 1, s.c_str());
        return out;
    }
    __declspec(dllexport) int cm_engine_history_remove(void* h, unsigned int idx)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->mgr && e->mgr->RemoveHistoryItem(idx)) ? 1 : 0;
    }
    __declspec(dllexport) void cm_engine_history_clear(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->ClearHistory(); } catch (...) {} }
    }
    __declspec(dllexport) unsigned long long cm_engine_history_maxsize(void* h)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        return (e && e->mgr) ? static_cast<unsigned long long>(e->mgr->MaxHistorySize()) : 0;
    }

    __declspec(dllexport) void cm_engine_set_history_load_mode(void* h, int enabled)
    {
        CMEngine* e = static_cast<CMEngine*>(h);
        if (e && e->mgr) { try { e->mgr->SetInHistoryItemLoadMode(enabled != 0); } catch (...) {} }
    }
}
