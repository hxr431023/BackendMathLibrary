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

## 目录结构

```
BackendMathLibrary/
├── LICENSE-MIT              # 引擎源码（微软 calculator）的 MIT 许可证
├── LICENSE-MPL-2.0          # 新增代码（api/tests/build）的 MPL-2.0 许可证
├── README.md
├── log.dll / config.dll / tmmf.dll / lerf.dll   # 随源码提交的运行时依赖
├── resources/cmd/
│   ├── en.lerf              # 英文语言文件
│   └── zh.lerf              # 中文语言文件
├── src/CalcManager/         # 引擎源码（源自 Microsoft/calculator）
├── api/                     # 纯 C 封装层
│   ├── calc_api.h           # 头文件（全部接口声明）
│   ├── calc_api.cpp         # 数学函数（弧度制 20 + 度数制 6）
│   ├── calc_engine.cpp      # 计算引擎封装（科学/程序员模式，按运算符优先级）
│   ├── calc_unit.cpp        # 单位换算（8 类，一步换算）
│   ├── calc_cmp.cpp         # 比较专区（数组输入/输出）
│   ├── calc_log.cpp         # log.dll 绑定（日志 + 缓存），自写代码
│   └── calc_native.cpp      # 模块目录解析 + config(tmmf) 设置 + lerf 双语，自写代码
├── tests/                   # 断言测试（test_calc / test_engine / test_unit /
│                            #   test_cmp / test_concurrency / test_settings / test_native）
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
- **返回指针生命周期**：
  - 数学接口返回的 `wchar_t*` 须用 `cm_free` 释放；
  - 引擎 / 单位查询返回的 `const wchar_t*`（类别名、单位名、单位缩写、`cm_unit_convert` 结果）
    在**下一次同类调用前**有效（句柄内单一共享缓冲）；
  - `cm_cmp_item` 的结果在**下一次比较前**有效；
  - `cm_msg` / `cm_msg_w` / `cm_get_language` / `cm_module_dir` 返回的指针在**下一次同类调用前**有效。

## 构建

`build/` 下的脚本使用 MSVC 命令行工具链编译（`/O2 /std:c++20 /utf-8` 等），
基于仓库根目录定位引擎源码（`src/CalcManager/CalcManager`）与封装源（`api/`），
产物输出到 `build/out/`。`vcvars64.bat` 路径可用参数 `-VcVars <路径>` 指定，未指定时自动探测常见位置；
构建完成后脚本会自动把 4 个依赖 DLL 与 `resources/cmd` 复制到 `build/out/`，产物目录可直接运行。

1. `build_lib_calcmgr.ps1`：编译引擎为静态库 `build/out/CalcManager.lib`
2. `build_dll_calcmgr.ps1`：把 6 个封装源（含 `calc_log.cpp`、`calc_native.cpp`）链接为 `build/out/CalcManager.dll`
3. `build_tests_calcmgr.ps1`：编译并运行 `tests/` 下的断言测试（按真实退出码判定，崩溃即失败）

## 下载

预编译的 64 位产物随每个 [Release](https://github.com/hxr431023/BackendMathLibrary/releases) 发布，
作为发布附件提供：`CalcManager.dll`、`CalcManager.lib`、`CalcManager.exp`，
以及运行时依赖 `log.dll`、`config.dll`、`tmmf.dll`、`lerf.dll`。
Release 页面同时附带 GitHub 自动生成的源码 zip/tar.gz。
