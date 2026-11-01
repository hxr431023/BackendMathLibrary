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
├── tests/                   # 功能验证（test_calc / test_engine / test_unit / test_cmp）
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

## 构建

`build/` 下的脚本使用 MSVC 命令行工具链编译（`/O2 /std:c++20 /utf-8` 等），
脚本内的源码/中间路径指向原作者本地目录，复制到本仓库后请按实际路径调整：

- `build_lib_calcmgr.ps1`：编译引擎为静态库 `CalcManager.lib`
- `build_dll_calcmgr.ps1`：把 4 个封装源 + `CalcManager.lib` 链接为 `CalcManager.dll`

`dist/` 已提供编译好的 64 位产物，可直接使用。
