// calc_api.h — C interface for CalcManager.dll (arbitrary-precision rational core).
// Include this header when consuming the DLL. Link against CalcManager_dll.lib.
#pragma once
#include <windows.h>

// ---- Command values (original CalculatorManager::Command) ----
#define CM_CMD_NULL     0
#define CM_CMD_SIGN     80
#define CM_CMD_CLEAR    81
#define CM_CMD_CENTR    82
#define CM_CMD_BACK     83
#define CM_CMD_PNT      84
#define CM_CMD_AND      86
#define CM_CMD_OR       87
#define CM_CMD_XOR      88
#define CM_CMD_LSHF     89
#define CM_CMD_RSHF     90
#define CM_CMD_DIV      91
#define CM_CMD_MUL      92
#define CM_CMD_ADD      93
#define CM_CMD_SUB      94
#define CM_CMD_MOD      95
#define CM_CMD_ROOT     96
#define CM_CMD_PWR      97
#define CM_CMD_CHOP     98
#define CM_CMD_ROL      99
#define CM_CMD_ROR      100
#define CM_CMD_COM      101
#define CM_CMD_SIN      102
#define CM_CMD_COS      103
#define CM_CMD_TAN      104
#define CM_CMD_SINH     105
#define CM_CMD_COSH     106
#define CM_CMD_TANH     107
#define CM_CMD_LN       108
#define CM_CMD_LOG      109
#define CM_CMD_SQRT     110
#define CM_CMD_SQR      111
#define CM_CMD_CUB      112
#define CM_CMD_FAC      113
#define CM_CMD_REC      114
#define CM_CMD_DMS      115
#define CM_CMD_CUBEROOT 116
#define CM_CMD_POW10    117
#define CM_CMD_PERCENT  118
#define CM_CMD_FE       119
#define CM_CMD_PI       120
#define CM_CMD_EQU      121
#define CM_CMD_MCLEAR   122
#define CM_CMD_RECALL   123
#define CM_CMD_STORE    124
#define CM_CMD_MPLUS    125
#define CM_CMD_MMINUS   126
#define CM_CMD_EXP      127
#define CM_CMD_OPENP    128
#define CM_CMD_CLOSEP   129
#define CM_CMD_0        130
#define CM_CMD_1        131
#define CM_CMD_2        132
#define CM_CMD_3        133
#define CM_CMD_4        134
#define CM_CMD_5        135
#define CM_CMD_6        136
#define CM_CMD_7        137
#define CM_CMD_8        138
#define CM_CMD_9        139
#define CM_CMD_A        140
#define CM_CMD_B        141
#define CM_CMD_C        142
#define CM_CMD_D        143
#define CM_CMD_E        144
#define CM_CMD_F        145
#define CM_CMD_INV      146
#define CM_CMD_SET_RESULT 147
#define CM_MODE_SCIENTIFIC  201
#define CM_CMD_ASIN    202
#define CM_CMD_ACOS    203
#define CM_CMD_ATAN    204
#define CM_CMD_POWE    205
#define CM_CMD_ASINH   206
#define CM_CMD_ACOSH   207
#define CM_CMD_ATANH   208
#define CM_MODE_PROGRAMMER 209
#define CM_CMD_HEX     313
#define CM_CMD_DEC     314
#define CM_CMD_OCT     315
#define CM_CMD_BIN     316
#define CM_CMD_QWORD   317
#define CM_CMD_DWORD   318
#define CM_CMD_WORD    319
#define CM_CMD_BYTE    320
#define CM_CMD_SEC     400
#define CM_CMD_ASEC    401
#define CM_CMD_CSC     402
#define CM_CMD_ACSC    403
#define CM_CMD_COT     404
#define CM_CMD_ACOT    405
#define CM_CMD_SECH    406
#define CM_CMD_ASECH   407
#define CM_CMD_CSCH    408
#define CM_CMD_ACSCH   409
#define CM_CMD_COTH    410
#define CM_CMD_ACOTH   411
#define CM_CMD_POW2    412
#define CM_CMD_ABS     413
#define CM_CMD_FLOOR   414
#define CM_CMD_CEIL    415
#define CM_CMD_ROLC    416
#define CM_CMD_RORC    417
#define CM_CMD_LOGBASEY 500
#define CM_CMD_NAND    501
#define CM_CMD_NOR     502
#define CM_CMD_RSHFL   505
#define CM_CMD_RAND    600
#define CM_CMD_EULER   601
// RadixType
#define CM_RADIX_HEX      0
#define CM_RADIX_DECIMAL  1
#define CM_RADIX_OCTAL    2
#define CM_RADIX_BINARY   3
// UnitConverter commands
#define CM_UC_ZERO       0
#define CM_UC_ONE        1
#define CM_UC_TWO        2
#define CM_UC_THREE      3
#define CM_UC_FOUR       4
#define CM_UC_FIVE       5
#define CM_UC_SIX        6
#define CM_UC_SEVEN      7
#define CM_UC_EIGHT      8
#define CM_UC_NINE       9
#define CM_UC_DECIMAL    10
#define CM_UC_NEGATE     11
#define CM_UC_BACKSPACE  12
#define CM_UC_CLEAR      13
#define CM_UC_RESET      14
#define CM_UC_NONE       15

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the engine. radix=10 for decimal, precision=digits of precision.
__declspec(dllimport) void cm_init(unsigned int radix, int precision);

// Arbitrary-precision arithmetic. All functions return a heap-allocated wide string
// (the result), or NULL on error. Release every returned string with cm_free().
__declspec(dllimport) wchar_t* cm_add(const wchar_t* a, const wchar_t* b);
__declspec(dllimport) wchar_t* cm_sub(const wchar_t* a, const wchar_t* b);
__declspec(dllimport) wchar_t* cm_mul(const wchar_t* a, const wchar_t* b);
__declspec(dllimport) wchar_t* cm_div(const wchar_t* a, const wchar_t* b);
__declspec(dllimport) wchar_t* cm_pow(const wchar_t* a, const wchar_t* b);
__declspec(dllimport) wchar_t* cm_sqrt(const wchar_t* a);

// Transcendental & misc (all arbitrary precision, radians for trig)
__declspec(dllimport) wchar_t* cm_sin(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_cos(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_tan(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_asin(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_acos(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_atan(const wchar_t* a);
// Degree-mode trig (arbitrary precision, angles in degrees)
__declspec(dllimport) wchar_t* cm_sin_deg(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_cos_deg(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_tan_deg(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_asin_deg(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_acos_deg(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_atan_deg(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_ln(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_log10(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_exp(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_fact(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_abs(const wchar_t* a);
__declspec(dllimport) wchar_t* cm_int(const wchar_t* a);

// Release a string previously returned by any cm_* function.
__declspec(dllimport) void cm_free(wchar_t* p);

// ---- Full-engine interface (original CalculatorManager) ----
// Handles are opaque; create with cm_engine_new, release with cm_engine_free.
__declspec(dllimport) void* cm_engine_new(void);
__declspec(dllimport) void cm_engine_free(void* h);
__declspec(dllimport) void cm_engine_reset(void* h, int clearMemory);
// Send any calculator command (use the CM_CMD_* / CM_MODE_* macros above).
__declspec(dllimport) void cm_engine_send_command(void* h, int command);
// Standard mode was removed: engine is scientific (operator precedence) by default.
__declspec(dllimport) void cm_engine_set_scientific_mode(void* h);
__declspec(dllimport) void cm_engine_set_programmer_mode(void* h);

// --- state queries (pointers are valid until the next command) ---
__declspec(dllimport) const wchar_t* cm_engine_get_display(void* h);
__declspec(dllimport) int cm_engine_is_error(void* h);
__declspec(dllimport) const wchar_t* cm_engine_get_expression(void* h);
__declspec(dllimport) unsigned int cm_engine_parenthesis_count(void* h);
__declspec(dllimport) int cm_engine_is_input_empty(void* h);
__declspec(dllimport) int cm_engine_is_recording(void* h);
__declspec(dllimport) int cm_engine_degree_mode(void* h);
__declspec(dllimport) int cm_engine_input_changed(void* h);

// --- radix / precision ---
__declspec(dllimport) void cm_engine_set_radix(void* h, int radix); // CM_RADIX_*
__declspec(dllimport) wchar_t* cm_engine_get_result_for_radix(void* h, unsigned int radix, int precision, int groupDigits);
__declspec(dllimport) void cm_engine_set_precision(void* h, int precision);
__declspec(dllimport) void cm_engine_update_max_int_digits(void* h);
__declspec(dllimport) wchar_t cm_engine_decimal_separator(void* h);

// --- memory ---
__declspec(dllimport) void cm_engine_memorize(void* h);
__declspec(dllimport) void cm_engine_memory_load(void* h, unsigned int index);
__declspec(dllimport) void cm_engine_memory_add(void* h, unsigned int index);
__declspec(dllimport) void cm_engine_memory_subtract(void* h, unsigned int index);
__declspec(dllimport) void cm_engine_memory_clear(void* h, unsigned int index);
__declspec(dllimport) void cm_engine_memory_clear_all(void* h);
__declspec(dllimport) int cm_engine_memory_count(void* h);
__declspec(dllimport) const wchar_t* cm_engine_memory_get(void* h, unsigned int index);

// --- history ---
__declspec(dllimport) int cm_engine_history_count(void* h);
__declspec(dllimport) wchar_t* cm_engine_history_item(void* h, unsigned int index); // "expr = result", free with cm_free
__declspec(dllimport) int cm_engine_history_remove(void* h, unsigned int index);
__declspec(dllimport) void cm_engine_history_clear(void* h);
__declspec(dllimport) unsigned long long cm_engine_history_maxsize(void* h);
__declspec(dllimport) void cm_engine_set_history_load_mode(void* h, int enabled);

// ---- Unit converter (original UnitConverter) ----
__declspec(dllimport) void* cm_unit_new(void);
__declspec(dllimport) void cm_unit_free(void* h);
__declspec(dllimport) int cm_unit_category_count(void* h);
__declspec(dllimport) const wchar_t* cm_unit_category_name(void* h, int idx);
__declspec(dllimport) void cm_unit_set_category(void* h, int idx);
__declspec(dllimport) int cm_unit_current_category_id(void* h);
__declspec(dllimport) int cm_unit_unit_count(void* h);
__declspec(dllimport) const wchar_t* cm_unit_unit_name(void* h, int idx);
__declspec(dllimport) const wchar_t* cm_unit_unit_abbrev(void* h, int idx);
// One-step convert value (in from unit idx) into to unit idx, no state change.
__declspec(dllimport) const wchar_t* cm_unit_convert(void* h, int catIdx, int fromIdx, int toIdx, const wchar_t* value);

// ---- Comparison zone (array in / array out) ----
// Input and output are both lists (arrays) of wide strings. Each result item is "key=value".
__declspec(dllimport) void* cm_cmp_new(void);
__declspec(dllimport) void cm_cmp_free(void* h);
// Compare plain numbers: inputs = {a, b, ...}. Returns result-item count (0 = failure).
__declspec(dllimport) int cm_cmp_compare(void* h, const wchar_t** inputs, int n);
// Compare quantities with units: inputs = {"1 kg", "500 g", ...}. Units are unified to the
// first input's unit, stripped, then compared. Returns result-item count (0 = failure).
__declspec(dllimport) int cm_cmp_compare_units(void* h, const wchar_t** inputs, int n);
// Result items of the last comparison (valid until the next compare call).
__declspec(dllimport) const wchar_t* cm_cmp_item(void* h, int idx);

#ifdef __cplusplus
}
#endif
