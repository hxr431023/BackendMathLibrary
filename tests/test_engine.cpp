// test_engine.cpp — verify the full-engine C interface against the real engine.
#include <cstdio>
#include "calc_api.h"

static void* h = nullptr;

static void send(int cmd) { cm_engine_send_command(h, cmd); }

static void show(const char* what)
{
    std::printf("%-28s display=%-14ls error=%d expr=%-10ls empty=%d\n",
                what, cm_engine_get_display(h), cm_engine_is_error(h),
                cm_engine_get_expression(h), cm_engine_is_input_empty(h));
}

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);   // unbuffered so crash point is visible
    h = cm_engine_new();
    if (!h) { std::printf("cm_engine_new FAILED\n"); return 1; }
    std::printf("engine created\n");

    // --- arithmetic with operator precedence: 2 + 3 * 4 = 14 ---
    send(CM_CMD_CLEAR);
    send(CM_CMD_2); send(CM_CMD_ADD); send(CM_CMD_3); send(CM_CMD_MUL); send(CM_CMD_4); send(CM_CMD_EQU);
    show("2+3*4=");
    std::printf("expected display = 14\n");

    // --- memory: memorize 5 (MS equivalent via memorize API) ---
    send(CM_CMD_CLEAR);
    send(CM_CMD_5); send(CM_CMD_EQU);
    cm_engine_memorize(h);
    std::printf("memory count=%d  mem[0]=%ls\n", cm_engine_memory_count(h), cm_engine_memory_get(h, 0));
    std::printf("expected mem[0] = 5\n");

    // --- history ---
    std::printf("history count=%d (max=%llu)\n", cm_engine_history_count(h), cm_engine_history_maxsize(h));
    wchar_t* item = cm_engine_history_item(h, 0);
    std::printf("history[0]=%ls\n", item ? item : L"<null>");
    cm_free(item);

    // --- radix: show 42 in hex ---
    send(CM_CMD_CLEAR);
    send(CM_CMD_4); send(CM_CMD_2); send(CM_CMD_EQU);
    wchar_t* hexv = cm_engine_get_result_for_radix(h, 16, 32, 0);
    std::printf("42 in hex = %ls (expected 2A)\n", hexv);
    cm_free(hexv);
    cm_engine_set_radix(h, CM_RADIX_HEX);
    show("after set_radix(hex)");

    // --- scientific mode: sqrt(16)=4, sin(0)=0 ---
    cm_engine_set_scientific_mode(h);
    send(CM_CMD_CLEAR);
    send(CM_CMD_1); send(CM_CMD_6); send(CM_CMD_SQRT); send(CM_CMD_EQU);
    show("sqrt(16)");
    std::printf("expected display = 4\n");

    send(CM_CMD_CLEAR);
    send(CM_CMD_0); send(CM_CMD_SIN); send(CM_CMD_EQU);
    show("sin(0)");
    std::printf("expected display = 0\n");

    // --- decimal separator / degree mode ---
    std::printf("decimal separator=%lc degree_mode=%d\n", cm_engine_decimal_separator(h), cm_engine_degree_mode(h));

    cm_engine_free(h);
    std::printf("engine freed\n");
    return 0;
}
