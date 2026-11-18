# BackendMathLibrary

一个基于 **微软开源计算器 `Microsoft/calculator`（CalcManager 引擎）** 编译而成的 **纯 C 接口数学库**。

动态库（DLL）与静态库（lib）均以标准 C ABI（`extern "C"`）导出，任何支持加载动态库的语言（C/C++、Python ctypes/cffi、Rust、Go、C# 等）都能直接调用。

> **来源与许可**：本仓库为**双重许可**——
> - 引擎源码 `src/CalcManager/` 源自微软开源仓库
>   [Microsoft/calculator](https://github.com/Microsoft/calculator)（`CalcManager` 模块），
>   遵循其 **MIT 许可**（`LICENSE-MIT`），已在每个引擎源文件末尾注明来源。
> - 新增的 `api/`、`tests/`、`build/` 代码采用 **MPL-2.0（Mozilla Public License 2.0）**（`LICENSE-MPL-2.0`）——
>   一种**轻度感染（弱 copyleft）**协议：把本库作为库使用、链接或调用的**上层代码无需开源**；
>   **仅当你修改了本库代码（Covered Software）**时，修改的部分才需以 MPL-2.0 公开。
>   每个自写源文件的第一行都注明「本文件为 BackendMathLibrary 自写代码（非 Microsoft/calculator 源码）」，
>   便于区分哪些是微软代码、哪些是本项目新增的代码。

## 依赖 DLL 自带，只从 DLL 所在目录加载

运行时依赖的 4 个 DLL **随源码一起提交**，放在仓库根目录：

| 文件 | 用途 |
| --- | --- |
| `log.dll` | 日志记录 + 操作结果缓存（GraalVM native-image，C ABI） |
| `config.dll` | 设置持久化，写入 `resources\cmd\settings.tmmf` |
| `tmmf.dll` | TMMF 配置文件格式支持 |
| `lerf.dll` | LERF 语言文件格式支持 |

加载规则（重要）：

- `CalcManager.dll` 用 `GetModuleHandleExW` 取得**自身所在目录**，所有依赖 DLL 与语言文件都只从该目录取。
- 代码里**没有任何绝对路径**，也不回落到 `C:\wpjc\md\native\DLL\` 之类的机器专属位置。
- 因此：下载者把 `CalcManager.dll` 与这 4 个 DLL、`resources\` 放在**同一个目录**，即可直接使用。
- 依赖 DLL 缺失时不会导致加载失败，相关能力静默降级为 no-op，数学接口照常工作。

## 可复用原生层（v0.5）

`api/native_runtime.h` + `api/native_runtime.cpp` 是与计算无关的通用层，**可以整对复制到其他项目**，
无需任何修改即可使用：里面没有一个绝对路径，所有依赖都在运行时按模块自身目录解析。

| 职责 | 接口 |
| --- | --- |
| 模块目录解析（`GetModuleHandleExW` + `GetModuleFileNameW`，只算一次） | `module_dir()` / `module_path(leaf)` |
| 从模块目录加载依赖 DLL | `load_beside(name)`（缺失返回 `nullptr`，静默降级） |
| TMMF 设置（config.dll：isolate + init + load + get/set/save） | `settings_init()` / `settings_ready()` / `settings_get` / `settings_set` / `settings_save` |
| LERF 语言文件解析（`<id>text<id>` 与 `key=value`，缺语言回落英文） | `parse_lerf_file()` / `load_lerf_table()` / `lerf_lookup()` |
| UTF-8 ↔ 宽字符 | `utf8_from_wide()` / `wide_from_utf8()` |

`calc_native.cpp` 现在只做三件事：导出 `cm_*`、保存当前语言与消息表、包装日志开关。
`calc_store_setting` / `calc_store_settings_save` / `calc_module_dir` 这三个内部符号保持原名，
因为 `calc_api.cpp` 与 `calc_log.cpp` 直接引用它们。

```cpp
#include "native_runtime.h"

bm_runtime::settings_init();
char buf[256] = {0};
bm_runtime::settings_get("language", buf, sizeof(buf));   // 返回值是状态码，不是长度
bm_runtime::settings_set("language", "zh");
bm_runtime::settings_save();
```

## 计算规律及答案（v0.6）

一张 **表达式 → 答案** 的表：常用算式的结果预先存好，命中时直接取用，不必再算一遍。
数据文件与设置一样使用 **TMMF 格式**（`#MDRenderer Settings` 头 + `#` 注释 + `key=value` 行）。

| 文件（均在 `resources\cmd\` 下） | 性质 | 是否入库 |
| --- | --- | --- |
| `calc_memo.tmmf` | **基线数据**，随源码分发，可直接编辑 | 是 |
| `calc_memo.user.tmmf` | 运行时学到的答案，机器状态 | 否（`.gitignore` 排除） |

键在读写前统一**归一化**：去掉所有空白、全角数字/字母转半角、`×` `÷` `−` 归一为 `*` `/` `-`、`π` 归一为 `pi`、
字母转小写、末尾的 `=` 去掉。因此 `1 + 1`、`１＋１`、`1+1=` 都会命中同一条。

| 接口 | 说明 |
| --- | --- |
| `cm_memo_init()` | 载入两张表，返回条目数（基线约 150 条）；可重复调用 |
| `cm_memo_lookup(expr, out, size)` | 查表：1 = 命中（答案写入 `out`），0 = 未命中 |
| `cm_memo_answer(expr)` | 同上，命中直接返回 UTF-8 答案指针，未命中返回 `NULL` |
| `cm_memo_store(expr, answer)` | 写入运行时表（内存），超出 4096 条时淘汰最旧一条 |
| `cm_memo_save()` | 把运行时表落盘为 `calc_memo.user.tmmf` |
| `cm_memo_forget(expr)` / `cm_memo_clear_runtime()` | 删除单条 / 清空运行时表（基线不动） |
| `cm_memo_normalize(expr, out, size)` | 取得该算式的规范键，便于界面展示或自建索引 |
| `cm_memo_item(idx, ...)` / `cm_memo_count()` / `cm_memo_hits(expr)` | 枚举条目 / 总数 / 单条命中次数 |
| `cm_memo_path(which)` | 表文件路径：0 = 基线，1 = 运行时 |
| `cm_memo_set_auto_store(on)` / `cm_memo_get_auto_store()` | 是否在每次 `CM_CMD_EQU` 之后自动记录（默认开启） |
| `cm_engine_memo_suggest(h, out, size)` | 打 `=` 之前先问一次表：1 = 已知答案，0 = 未知，照常计算 |

自动记录走引擎的等号命令：`cm_engine_send_command(h, CM_CMD_EQU)` 算出结果后，
表达式与答案会写入运行时表（`1+1` 这类"算式即答案"的输入会被跳过）。
`suggest` 只读不改引擎状态，宿主可以据此省掉一次求值，也可以忽略它照常 `CM_CMD_EQU`。

```c
cm_memo_init();                                   // 载入基线 + 运行时表

char out[256] = {0};
if (cm_memo_lookup("10!", out, sizeof(out))) {    // 1 = 命中，答案 "3628800"
    /* 直接用 out，不需要求值 */
}

cm_engine_send_command(h, CM_CMD_2);              // 输入 2 + 3 =
cm_engine_send_command(h, CM_CMD_ADD);
cm_engine_send_command(h, CM_CMD_3);
cm_engine_send_command(h, CM_CMD_EQU);            // 算完自动记录 "2+3=5"
cm_memo_save();                                   // 落盘到 calc_memo.user.tmmf

cm_memo_lookup("2+3", out, sizeof(out));          // 下次直接命中 "5"
```

## 目录结构

```
BackendMathLibrary/
├── LICENSE-MIT              # 引擎源码（微软 calculator）的 MIT 许可证
├── LICENSE-MPL-2.0          # 新增代码（api/tests/build）的 MPL-2.0 许可证
├── README.md
├── log.dll / config.dll / tmmf.dll / lerf.dll   # 随源码提交的运行时依赖
├── resources/cmd/
│   ├── en.lerf              # 英文语言文件
│   ├── zh.lerf              # 中文语言文件
│   ├── calc_memo.tmmf       # 计算规律及答案：基线数据（随源码分发）
│   └── calc_memo.user.tmmf  # 运行时学到的答案（机器状态，不入库）
├── src/CalcManager/         # 引擎源码（源自 Microsoft/calculator）
├── api/                     # 纯 C 封装层
│   ├── calc_api.h           # 头文件（全部接口声明）
│   ├── calc_api.cpp         # 数学函数（弧度制 20 + 度数制 6）
│   ├── calc_engine.cpp      # 计算引擎封装（科学/程序员模式，按运算符优先级）
│   ├── calc_unit.cpp        # 单位换算（8 类，一步换算）
│   ├── calc_cmp.cpp         # 比较专区（数组输入/输出）
│   ├── calc_memo.h          # 计算规律及答案表（归一化 / 查表 / 学习 / 落盘），自写代码
│   ├── calc_memo.cpp        #   同上实现，TMMF 双文件，自写代码
│   ├── calc_log.cpp         # log.dll 绑定（日志 + 缓存），自写代码
│   ├── native_runtime.h     # 可复用原生层（模块目录 / 依赖 DLL / TMMF 设置 / LERF），自写代码
│   ├── native_runtime.cpp   #   同上实现，与本项目无关，可直接复制到其他项目
│   └── calc_native.cpp      # 仅保留 cm_* 导出 + 语言状态 + 日志开关，自写代码
├── tests/                   # 断言测试（test_calc / test_engine / test_unit / test_cmp /
│                            #   test_concurrency / test_settings / test_native / test_memo）
└── build/                   # 构建脚本（MSVC 命令行）
```

本仓库**不保存编译产物**。DLL / lib / exp 与源码压缩包统一放在
[Releases](https://github.com/hxr431023/BackendMathLibrary/releases) 页面，作为发布附件提供；
构建目录 `build/out/` 是本地生成的中间结果，已在 `.gitignore` 中排除。

## 接口概览

- **计算引擎**：科学/程序员模式，运算**按运算符优先级**（`2+3*4=14`）。
- **数学函数**：`cm_sin/cm_cos/...`（弧度制）+ `cm_sin_deg/...`（度数制）。
- **单位换算**：8 类（Length/Mass/Temperature/Speed/Area/Volume/Time/Data），`cm_unit_convert` 一步换算。
- **比较专区**：`cm_cmp_compare`（无单位）/ `cm_cmp_compare_units`（带单位，统一到首项单位后比较），
  输入输出均为宽字符串数组，每项 `"key=value"`。
- **计算规律及答案**：`cm_memo_lookup` / `cm_memo_store` / `cm_engine_memo_suggest`，
  常用算式结果预先存表，命中即取用；详见下方专节。

### 运行时设置（TMMF）

设置内容一律存 TMMF（`key=value` 文本，含 `#MDRenderer Settings` 头与时间戳注释），
由 `config.dll` 自动落到 `<CalcManager.dll 所在目录>\resources\cmd\settings.tmmf`。

| 接口 | 说明 |
| --- | --- |
| `cm_native_init()` | 加载同目录的 config/tmmf/lerf DLL 并读取已存设置；返回 1 表示可用 |
| `cm_settings_get(key, out, size)` | 读取设置值；**返回状态码而非长度，值从 `out` 取** |
| `cm_settings_set(key, value)` | 只改内存，需 `cm_settings_save()` 才落盘 |
| `cm_settings_save()` | 写 `settings.tmmf` |
| `cm_set_output_format(radix, precision, scientific)` | 当前线程的进制/精度/表示法，并持久化为默认值 |
| `cm_log_start()` / `cm_log_stop()` | 从 DLL 目录加载 / 关闭 `log.dll`（无需传路径） |
| `cm_set_log_enabled(enable)` | 打开或关闭每次调用的日志记录；缓存不受开关影响 |

已落盘的设置项：`language`、`log_enabled`、`radix`、`precision`、`scientific`。

### 语言文件（LERF，中英双语）

- 语言文件是 LERF 标准格式：首行 `LERF.utf8`，第二行版本串，第三行 `0`，
  之后每条为 `<id>文本<id>`。
- 本项目提供 `resources/cmd/en.lerf` 与 `resources/cmd/zh.lerf` 两套，条目一一对应。
- `cm_set_language("en"|"zh")` 切换并持久化，`cm_get_language()` 查询当前语言。
- `cm_msg(id)` / `cm_msg_w(id)` 按 id 取本地化文案，UTF-8 / 宽字符两种形式，查不到时原样返回 id。
- 说明：这版 `lerf.dll` 的取值函数只回显 key（`lerf_key_count` 恒为 0），
  所以 LERF 的**解析在本项目侧完成**（`parse_lerf()`），`lerf.dll` 仅作为格式支持库加载。

## 使用

```c
#include "calc_api.h"
#pragma comment(lib, "CalcManager_dll.lib")

// 数学：2^100 精确整数
cm_init(10, 32);
wchar_t* r = cm_pow(L"2", L"100");   // 1267650600228229401496703205376
cm_free(r);

// 初始化运行时依赖（同目录查找，不需要任何路径参数）
cm_native_init();

// 设置：日志与输出格式
cm_log_start();
cm_set_log_enabled(1);
wchar_t* r2 = cm_add(L"2", L"3");    // log.dll 记录: cm_add(2,3) = 5
cm_free(r2);
cm_set_log_enabled(0);
cm_log_stop();

cm_set_output_format(16, 8, 0);      // 十六进制、8 位有效数字、普通表示
r2 = cm_add(L"A", L"5");             // "F"
cm_free(r2);
cm_set_output_format(10, 32, 0);     // 恢复默认

// 语言：中英切换 + 取本地化文案
cm_set_language("zh");
const wchar_t* msg = cm_msg_w("error.divide_by_zero");   // 除数不能为零
cm_set_language("en");

// 设置读写
char buf[64] = {0};
cm_settings_get("radix", buf, sizeof(buf));   // buf = "10"，返回值只是状态码
cm_settings_set("precision", "32");
cm_settings_save();

// 单位换算：1 m -> km
void* u = cm_unit_new();
const wchar_t* v = cm_unit_convert(u, 0, 0, 1, L"1");  // 0.001
cm_unit_free(u);

// 比较：1 kg vs 500 g（带单位，统一到 kg）
void* c = cm_cmp_new();
int n = cm_cmp_compare_units(c, (const wchar_t*[]){ L"1 kg", L"500 g" }, 2);
// cm_cmp_item(c, i) -> "value1=0.5"、"result=greater" ...
cm_cmp_free(c);
```

## 线程安全与精度说明

- **线程模型（真并发）**：库运行于**单进程**，每个 `cm_*` 操作在独立线程中运行，线程用完自动回收。
  引擎的关键全局常量（Ratpak 的 `num_one`…`pi`…`rat_min_i32` 等、`g_ratio`、`g_decimalSeparator`、
  `g_ftrueinfinite`）与封装的 `radix / precision` 均声明为 **`thread_local`**，线程间互不干扰；
  数学接口返回的字符串在堆上分配、由调用方释放，不共享可变全局缓冲。
  因此**可同时开多个线程并行**调用数学 `cm_*` 函数而无需加锁（已附 8 线程并发冒烟测试验证）。
  输出格式（radix / precision / scientific）为 `thread_local`，各线程可独立配置互不干扰。
  日志开关（`cm_set_log_enabled`）为全局设置，所有线程共享。
  `cm_engine_*`、`cm_unit_*`、`cm_cmp_*` 各自持有句柄（每实例独立状态），同一实例勿在多个线程中并发使用。
- **单位换算精度**：`cm_unit_convert` 与比较专区使用 **`double` 精度**（非任意精度）；
  极小的非零数值不会被抹零，比较判等采用容差（1e-9）。引擎算术运算本身为任意精度。
- **计算规律及答案表**：`cm_memo_*` 内部用互斥锁保护两张表，可在多个线程并发查询/写入；
  `cm_memo_answer` 返回的指针在**本线程下一次 `cm_memo_answer` 前**有效。
- **返回指针生命周期**：
  - 数学接口返回的 `wchar_t*` 须用 `cm_free` 释放；
  - 引擎 / 单位查询返回的 `const wchar_t*`（类别名、单位名、单位缩写、`cm_unit_convert` 结果）
    在**下一次同类调用前**有效（句柄内单一共享缓冲）；
  - `cm_cmp_item` 的结果在**下一次比较前**有效；
  - `cm_msg` / `cm_msg_w` / `cm_get_language` / `cm_module_dir` 返回的指针在**下一次同类调用前**有效；
  - `cm_memo_answer` / `cm_memo_path` 返回的指针在本线程下一次同类调用前有效。

## 构建

`build/` 下的脚本使用 MSVC 命令行工具链编译（`/O2 /std:c++20 /utf-8` 等），
基于仓库根目录定位引擎源码（`src/CalcManager/CalcManager`）与封装源（`api/`），
产物输出到 `build/out/`。`vcvars64.bat` 路径可用参数 `-VcVars <路径>` 指定，未指定时自动探测常见位置；
构建完成后脚本会自动把 4 个依赖 DLL 与 `resources/cmd`（两份 LERF 语言文件 + `calc_memo.tmmf` 基线数据）
复制到 `build/out/`，产物目录可直接运行。

1. `build_lib_calcmgr.ps1`：编译引擎为静态库 `build/out/CalcManager.lib`
2. `build_dll_calcmgr.ps1`：把 8 个封装源（含 `calc_log.cpp`、`calc_native.cpp`、`calc_memo.cpp`）链接为 `build/out/CalcManager.dll`
3. `build_tests_calcmgr.ps1`：编译并运行 `tests/` 下的断言测试（按真实退出码判定，崩溃即失败）

## 下载

预编译的 64 位产物随每个 [Release](https://github.com/hxr431023/BackendMathLibrary/releases) 发布，
作为发布附件提供。v0.6 的附件共 10 个：

| 附件 | 说明 |
| --- | --- |
| `CalcManager.dll` | 主库（纯 C ABI；v0.6 新增计算规律及答案接口） |
| `CalcManager.lib` | 静态库（引擎部分，构建用） |
| `CalcManager.exp` | 链接导出文件 |
| `log.dll` | 日志 + 操作缓存（运行时依赖） |
| `config.dll` | TMMF 设置持久化（运行时依赖） |
| `tmmf.dll` | TMMF 格式支持（运行时依赖） |
| `lerf.dll` | LERF 格式支持（运行时依赖） |
| `en.lerf` | 英文语言文件（`resources\cmd\`） |
| `zh.lerf` | 中文语言文件（`resources\cmd\`） |
| `calc_memo.tmmf` | 计算规律及答案基线数据（`resources\cmd\`） |

把这 10 个文件放到**同一个目录**（`en.lerf` / `zh.lerf` / `calc_memo.tmmf` 放进该目录下的
`resources\cmd\`）即可直接使用，不需要任何路径配置。Release 页面同时附带 GitHub 自动生成的源码 zip/tar.gz。
