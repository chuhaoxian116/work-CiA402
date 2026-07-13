# C++ 命名、注释与格式规范

本文档只描述通用 C++ 代码命名、注释和格式化规则，不包含具体业务模块说明。

## 1. 命名规则

```text
类型名：PascalCase
函数名：PascalCase
普通变量 / 函数参数：snake_case
常量：kPascalCase
枚举值：kPascalCase
命名空间：snake_case
文件名：lower_snake_case
缩进：2 空格
```

示例：

```cpp
namespace motion_control {

enum class AxisMode : int8_t {
  kHoming = 6,
  kCyclicSynchronousPosition = 8
};

Status SwitchMode(AxisData& axis, AxisMode target_mode);

}  // namespace motion_control
```

## 2. 头文件注释规则

头文件用于对外发布类型和接口，因此类型、结构体、函数使用 Doxygen 块注释。

函数注释格式：

```cpp
/**
 * @brief 获取轴状态。
 *
 * 根据输入数据解析当前轴状态，并缓存解析结果。
 *
 * @param axis 单轴数据对象。
 * @return AxisState 当前轴状态。
 */
AxisState GetAxisState(AxisData& axis);
```

结构体注释格式：

```cpp
/**
 * @brief 单轴输入数据。
 *
 * 由上层在调用接口前写入。
 */
struct AxisInput {
  uint16_t statusword = 0;  // 状态字。
  int8_t mode_display = 0;  // 当前模式反馈。
};
```

枚举注释格式：

```cpp
/**
 * @brief 轴运行模式。
 */
enum class AxisMode : int8_t {
  kHoming = 6,                    // 回零模式。
  kCyclicSynchronousPosition = 8,  // 周期同步位置模式。
};
```

枚举值和成员变量使用普通 `//` 注释，不使用 `///<`：

```cpp
kHoming = 6,  // 正确。
kHoming = 6,  ///< 不采用。
```

## 3. 源文件注释规则

`.cpp` 文件只使用普通 `//` 注释，不使用 Doxygen 块注释。

正确：

```cpp
AxisState GetAxisState(AxisData& axis) {
  // 缓存本周期状态解析结果，便于后续逻辑复用。
  axis.axis_state = DecodeAxisState(axis.input.statusword);
  return axis.axis_state;
}
```

不采用：

```cpp
AxisState GetAxisState(AxisData& axis) {
  /**
   * @brief 缓存本周期状态解析结果。
   */
  axis.axis_state = DecodeAxisState(axis.input.statusword);
  return axis.axis_state;
}
```

内部常量使用普通 `//` 注释：

```cpp
constexpr uint16_t kStartBit = 0x0010;  // 启动位。
```

## 4. 注释内容原则

注释应说明“为什么这样做”或“这段逻辑的边界”，不要重复代码本身。

推荐：

```cpp
// 模式未切换完成时不继续下发启动命令，避免状态机提前动作。
if (current_mode != target_mode) {
  return Status::kBusy;
}
```

不推荐：

```cpp
// 判断 current_mode 是否不等于 target_mode。
if (current_mode != target_mode) {
  return Status::kBusy;
}
```

## 5. 格式化规则

推荐使用 `.clang-format` 管理 C/C++ 代码格式。

推荐配置：

```yaml
BasedOnStyle: Google
Language: Cpp
IndentWidth: 2
ColumnLimit: 100
PointerAlignment: Left
DerivePointerAlignment: false
AllowShortFunctionsOnASingleLine: Empty
SortIncludes: false
```

终端格式化命令：

```bash
clang-format -i include/example.h src/example.cpp
```

如果文件较多，推荐增加 `scripts/format.sh`：

```bash
#!/usr/bin/env bash
set -e

find include src examples \
  \( -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" \) \
  -print0 | xargs -0 clang-format -i
```

## 6. VSCode 设置建议

VSCode 保存格式化应读取工程内 `.clang-format`。

推荐设置：

```json
{
  "editor.formatOnSave": true,
  "C_Cpp.clang_format_style": "file"
}
```

这样 VSCode 保存和终端 `clang-format -i` 会使用同一套规则。

## 7. 检查命令

检查 `.cpp` 是否误用了 Doxygen 注释：

```bash
rg -n "///<|/\\*\\*|@brief" src
```

检查头文件是否出现不需要的行尾 Doxygen 注释：

```bash
rg -n "///<" include
```

格式化后建议至少执行一次编译：

```bash
cmake --build build
```
