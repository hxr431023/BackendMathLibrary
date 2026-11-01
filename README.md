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

## 目录结构

```
BackendMathLibrary/
├── LICENSE-MIT             # 引擎源码（微软 calculator）的 MIT 许可证
├── LICENSE-MPL-2.0         # 新增代码（api/tests/build）的 MPL-2.0 许可证
├── README.md
├── src/CalcManager/         # 引擎源码（源自 Microsoft/calculator）
├── api/                     # 纯 C 封装层
│   ├── calc_api.h           # 头文件（全部接口声明）
│   ├── calc_api.cpp         # 数学函数（弧度制 20 + 度数制 6）
│   ├── calc_engine.cpp      # 计算引擎封装（科学/程序员模式，按运算符优先级）
│   ├── calc_unit.cpp        # 单位换算（8 类，一步换算）
│   └── calc_cmp.cpp         # 比较专区（数组输入/输出）
├── tests/                   # 功能验证（test_calc / test_engine / test_unit / test_cmp / test_concurrency）
├── build/                   # 构建脚本（MSVC 命令行）
└── dist/                    # 编译产物
    ├── CalcManager.dll
    ├── CalcManager.lib
    └── CalcManager_dll.lib
```

## 接口概览（74 个导出）

- **计算引擎**：科学/程序员模式，运算**按运算符优先级**（`2+3*4=14`）。
- **数学函数**：`cm_sin/cm_cos/...`（弧度制）+ `cm_sin_deg/...`（度数制）。
- **单位换算**：8 类（Length/Mass/Temperature/Speed/Area/Volume/Time/Data），`cm_unit_convert` 一步换算。
- **比较专区**：`cm_cmp_compare`（无单位）/ `cm_cmp_compare_units`（带单位，统一到首项单位后比较），
  输入输出均为宽字符串数组，每项 `"key=value"`。

详见 `api/calc_api.h` 与各测试文件。

## 使用

```c
#include "calc_api.h"
#pragma comment(lib, "CalcManager_dll.lib")

// 数学：2^100 精确整数
cm_init(10, 32);
wchar_t* r = cm_pow(L"2", L"100");   // 1267650600228229401496703205376
cm_free(r);

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
  `cm_engine_*`、`cm_unit_*`、`cm_cmp_*` 各自持有句柄（每实例独立状态），同一实例勿在多个线程中并发使用。
- **单位换算精度**：`cm_unit_convert` 与比较专区使用 **`double` 精度**（非任意精度）；
  极小的非零数值不会被抹零，比较判等采用容差（1e-9）。引擎算术运算本身为任意精度。
- **返回指针生命周期**：
  - 数学接口返回的 `wchar_t*` 须用 `cm_free` 释放；
  - 引擎 / 单位查询返回的 `const wchar_t*`（类别名、单位名、单位缩写、`cm_unit_convert` 结果）
    在**下一次同类调用前**有效（句柄内单一共享缓冲）；
  - `cm_cmp_item` 的结果在**下一次比较前**有效。

## 构建

`build/` 下的脚本使用 MSVC 命令行工具链编译（`/O2 /std:c++20 /utf-8` 等），
基于仓库根目录定位引擎源码（`src/CalcManager/CalcManager`）与封装源（`api/`），
产物输出到 `build/out/`。`vcvars64.bat` 路径可用参数 `-VcVars <路径>` 指定，未指定时自动探测常见位置。

1. `build_lib_calcmgr.ps1`：编译引擎为静态库 `build/out/CalcManager.lib`
2. `build_dll_calcmgr.ps1`：把 4 个封装源链接 `CalcManager.lib` 为 `build/out/CalcManager.dll`
3. `build_tests_calcmgr.ps1`：编译并运行 `tests/` 下的断言测试（非零退出码表示失败）

`dist/` 已提供编译好的 64 位产物，可直接使用。
