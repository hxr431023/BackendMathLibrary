// test_unit.cpp — verify the unit-converter C interface (built-in unit data).
// Conversions use the stateless one-step cm_unit_convert.
#include <cstdio>
#include "calc_api.h"

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    void* u = cm_unit_new();
    if (!u) { std::printf("cm_unit_new FAILED\n"); return 1; }

    std::printf("categories=%d\n", cm_unit_category_count(u));
    for (int i = 0; i < cm_unit_category_count(u); ++i)
        std::printf("  cat[%d] = %ls\n", i, cm_unit_category_name(u, i));

    // One-step conversions: cm_unit_convert(h, catIdx, fromIdx, toIdx, value)
    std::printf("1 m = %ls km (expected 0.001)\n", cm_unit_convert(u, 0, 0, 1, L"1"));       // Length Meter->Kilometer
    std::printf("0 C = %ls F (expected 32)\n", cm_unit_convert(u, 2, 0, 1, L"0"));           // Temperature Celsius->Fahrenheit
    std::printf("100 C = %ls K (expected 373.15)\n", cm_unit_convert(u, 2, 0, 2, L"100"));  // Temperature Celsius->Kelvin
    std::printf("1 h = %ls min (expected 60)\n", cm_unit_convert(u, 6, 2, 1, L"1"));        // Time Hour->Minute
    std::printf("1 kg = %ls lb (expected 2.2046)\n", cm_unit_convert(u, 1, 0, 4, L"1"));    // Mass Kilogram->Pound

    cm_unit_free(u);
    std::printf("unit converter freed\n");
    return 0;
}
