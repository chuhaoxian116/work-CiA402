# CiA402 命名与格式规范

本文档用于统一 `CiA402` 动态库后续代码风格。本工程采用接近 Google C++ Style 的命名规则：

```text
类型名：PascalCase
函数名：PascalCase
普通变量 / 函数参数：snake_case
类成员变量：snake_case_
结构体成员：snake_case
常量：kPascalCase
枚举值：kPascalCase
命名空间：snake_case
文件名：lower_snake_case
```

注意：这和很多项目常用的 `camelCase` 不一样。后续不要再混用 `getAxisState()`、`get_axis_state()`、`GetAxisState()` 三套风格。本项目 C++ 主接口统一使用：

```cpp
GetAxisState()
GetAxisHomingState()
PowerAxis::Update()
SwitchMode::Update()
```

## 1. 工程边界

这个库只负责 CiA402 状态控制：

```text
状态解析
模式切换
使能 / 断使能
清错
Homing 启停
```

不负责：

```text
位置 / 速度 / 力矩
单位换算
减速比
轨迹规划
插补
IgH / SOEM PDO offset
SDO 参数下发
厂商私有对象
```

## 2. 文件命名

文件名使用小写加下划线。

```text
推荐：
  cia402.h
  cia402.cpp
  basic_usage.cpp
  naming_style.md

不推荐：
  CiA402.hpp
  Cia402.cpp
  BasicUsage.cpp
  namingStyle.md
```

Google C++ 官方更偏好 `.cc`，但当前工程已经使用 `.cpp`，所以本项目保持 `.cpp`。

建议目录：

```text
CiA402/
  include/cia402/
    cia402.h          # C++ 对外头文件；如果只做 C++，这个文件也可以承载 C++ 接口
  src/
    cia402.cpp
  examples/
    basic_usage.cpp
  README.md
  NAMING_STYLE.md
```

如果后续还需要 C ABI 兼容层，建议单独拆成：

```text
include/cia402/cia402_c.h
```

不要把 C 风格接口和 C++ 主接口混在一个头文件里。

## 3. 命名空间

命名空间使用小写蛇形。

```cpp
namespace cia402 {

// ...

}  // namespace cia402
```

当前库不绑定 IgH/SOEM，所以 namespace 不应出现：

```text
igh
soem
ethercat
siasun
robot
```

## 4. 类型命名

类、结构体、枚举、类型别名统一使用 PascalCase。

```cpp
enum class AxisMode;
enum class AxisState;
enum class AxisHomingState;
enum class FbStatus;

struct AxisInput;
struct AxisOutput;

class PowerAxis;
class ClearAxisError;
class SwitchMode;
class Homing;
```

不推荐：

```cpp
axis_input_t       // C++ 主接口不使用
AxisState402       // 库名已经是 cia402，不需要重复 402
AxisHomeState      // 使用 Homing，更贴近 CiA402 标准术语
FBStatus           // 缩写全大写不采用
```

如果保留 C ABI，C 类型可以使用 `_t`：

```c
cia402_axis_input_t
cia402_axis_output_t
```

但这只属于 C ABI 层，不属于 C++ 主接口。

## 5. 枚举命名

枚举类型使用 PascalCase，枚举值使用 `kPascalCase`。

```cpp
enum class AxisMode : int8_t {
  kNone = 0,
  kProfilePosition = 1,
  kVelocity = 2,
  kProfileVelocity = 3,
  kProfileTorque = 4,
  kHoming = 6,
  kInterpolatedPosition = 7,
  kCyclicSyncPosition = 8,
  kCyclicSyncVelocity = 9,
  kCyclicSyncTorque = 10,
};
```

```cpp
enum class AxisState {
  kNotReadyToSwitchOn,
  kSwitchOnDisabled,
  kReadyToSwitchOn,
  kSwitchedOn,
  kOperationEnabled,
  kQuickStopActive,
  kFaultReactionActive,
  kFault,
  kUnknown,
};
```

```cpp
enum class AxisHomingState {
  kNotHomingMode,
  kNotStarted,
  kRunning,
  kAttained,
  kError,
  kUnknown,
};
```

不推荐新代码使用：

```cpp
enum class AxisState {
  OperationEnabled,   // 缺少 k 前缀
  OPERATION_ENABLED,  // 宏风格，不用于 C++ 枚举
};
```

## 6. 函数命名

普通函数和成员函数统一使用 PascalCase。

```cpp
AxisState GetAxisState(const AxisInput& input);
AxisHomingState GetAxisHomingState(const AxisInput& input);

bool IsFault(const AxisInput& input);
bool IsWarning(const AxisInput& input);
bool IsRemote(const AxisInput& input);
bool IsOperationEnabled(const AxisInput& input);
bool IsTargetReached(const AxisInput& input);
bool IsModeReached(const AxisInput& input, AxisMode target_mode);

const char* ToString(AxisMode mode);
const char* ToString(AxisState state);
const char* ToString(AxisHomingState state);
const char* ToString(FbStatus status);
```

不推荐：

```cpp
getAxisState()      // camelCase，不采用
get_axis_state()    // 普通 C++ 函数不使用 snake_case
axisState()         // 语义不够清楚
```

### Getter / Setter 例外

如果后续类里出现简单访问器，可以使用变量风格：

```cpp
class AxisStatus {
 public:
  uint16_t statusword() const;
  int8_t mode_display() const;
};
```

这类访问器可以不写成：

```cpp
GetStatusword()
GetModeDisplay()
```

但普通功能函数仍然使用 PascalCase。

## 7. Function Block 命名

功能块使用类表达动作，类名 PascalCase。

```cpp
class PowerAxis;
class ClearAxisError;
class SwitchMode;
class Homing;
```

单周期执行函数统一叫 `Update()`。

```cpp
class PowerAxis {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  bool enable);
};
```

```cpp
class SwitchMode {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  AxisMode target_mode);
};
```

不要混用不同动词：

```cpp
PowerAxis::Run()
ClearAxisError::Step()
SwitchMode::Execute()
Homing::Process()
```

统一 `Update()`，实时周期里每周期调用一次，语义最清楚。

## 8. 变量、参数和成员变量

普通变量和函数参数使用 snake_case。

```cpp
int axis_count = 6;
int current_axis_index = 0;
bool communication_ready = false;

bool ConfigureAxis(int axis_index, int64_t cycle_time_ns);
```

类成员变量使用 snake_case，并在末尾加 `_`。

```cpp
class PowerAxis {
 private:
  bool enabled_;
  AxisState last_state_;
};
```

结构体成员不加尾部 `_`。

```cpp
struct AxisInput {
  uint16_t statusword;     // 0x6041 Statusword
  int8_t mode_display;     // 0x6061 Modes of operation display
};

struct AxisOutput {
  uint16_t controlword;    // 0x6040 Controlword
  int8_t mode;             // 0x6060 Modes of operation
};
```

本项目固定使用：

```text
statusword
controlword
mode_display
```

不要同时出现：

```text
statusword
status_word
statusWord
```

`Statusword` 和 `Controlword` 在 CiA402 标准中本身就是对象名，因此这里保留为一个词。

## 9. 常量命名

常量使用 `kPascalCase`。

```cpp
constexpr uint16_t kControlwordShutdown = 0x0006;
constexpr uint16_t kControlwordSwitchOn = 0x0007;
constexpr uint16_t kControlwordEnableOperation = 0x000F;
constexpr uint16_t kControlwordFaultReset = 0x0080;
```

函数内部由输入计算出来的 `const` 变量，不使用 `k`。

```cpp
const AxisState state = GetAxisState(input);
```

真正固定不变的编译期常量才使用 `k`。

## 10. 宏命名

宏使用全大写加下划线，并尽量带项目前缀。

```cpp
CIA402_API
CIA402_MAX_AXIS_COUNT
```

但能用 `constexpr`、`inline`、`enum class` 的地方不要使用宏。

## 11. 返回值命名

Function block 返回值统一为：

```cpp
enum class FbStatus {
  kDone,
  kBusy,
  kError,
};
```

含义：

```text
kDone   当前请求已经完成
kBusy   当前请求还在进行，需要下个周期继续调用
kError  当前状态无法继续，需要上层处理
```

不要使用：

```text
Success / Running / Failed
Ok / Wait / Fault
0 / 1 / -1
```

这些容易和驱动器 Fault、通信 Error、函数执行错误混在一起。

## 12. 注释风格

对外接口必须写清楚对象字典编号。

```cpp
struct AxisInput {
  uint16_t statusword;     // 0x6041 Statusword
  int8_t mode_display;     // 0x6061 Modes of operation display
};
```

Function block 注释必须说明：

```text
1. 输入是什么
2. 输出会改什么
3. 返回 kDone / kBusy / kError 的条件
4. 是否阻塞
5. 是否访问主站或 PDO 内存
```

本库 Function Block 固定原则：

```text
不阻塞
不 sleep
不访问 IgH/SOEM
不直接读写 PDO 内存
每周期调用一次
```

## 13. 缩进和格式

本项目使用 Google C++ 格式：

```text
缩进：2 个空格
Tab：禁止
左大括号：放在当前行末尾
每行长度：尽量不超过 80 字符
if/for/while：建议始终使用大括号
```

示例：

```cpp
bool ConfigureAxis(int axis_index, int64_t cycle_time_ns) {
  if (axis_index < 0) {
    return false;
  }

  return true;
}
```

类定义：

```cpp
class PowerAxis {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  bool enable);

 private:
  bool enabled_ = false;
};
```

## 14. 推荐 C++ 主接口草案

如果后续统一成纯 C++，建议最终对外头文件接近下面这样：

```cpp
#ifndef CIA402_CIA402_H_
#define CIA402_CIA402_H_

#include <cstdint>

namespace cia402 {

enum class AxisMode : int8_t {
  kNone = 0,
  kProfilePosition = 1,
  kVelocity = 2,
  kProfileVelocity = 3,
  kProfileTorque = 4,
  kHoming = 6,
  kInterpolatedPosition = 7,
  kCyclicSyncPosition = 8,
  kCyclicSyncVelocity = 9,
  kCyclicSyncTorque = 10,
};

enum class AxisState {
  kNotReadyToSwitchOn,
  kSwitchOnDisabled,
  kReadyToSwitchOn,
  kSwitchedOn,
  kOperationEnabled,
  kQuickStopActive,
  kFaultReactionActive,
  kFault,
  kUnknown,
};

enum class AxisHomingState {
  kNotHomingMode,
  kNotStarted,
  kRunning,
  kAttained,
  kError,
  kUnknown,
};

enum class FbStatus {
  kDone,
  kBusy,
  kError,
};

struct AxisInput {
  uint16_t statusword;     // 0x6041 Statusword
  int8_t mode_display;     // 0x6061 Modes of operation display
};

struct AxisOutput {
  uint16_t controlword;    // 0x6040 Controlword
  int8_t mode;             // 0x6060 Modes of operation
};

AxisState GetAxisState(const AxisInput& input);
AxisHomingState GetAxisHomingState(const AxisInput& input);

bool IsFault(const AxisInput& input);
bool IsWarning(const AxisInput& input);
bool IsRemote(const AxisInput& input);
bool IsOperationEnabled(const AxisInput& input);
bool IsTargetReached(const AxisInput& input);
bool IsModeReached(const AxisInput& input, AxisMode target_mode);

const char* ToString(AxisMode mode);
const char* ToString(AxisState state);
const char* ToString(AxisHomingState state);
const char* ToString(FbStatus status);

class PowerAxis {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  bool enable);
};

class ClearAxisError {
 public:
  FbStatus Update(const AxisInput& input, AxisOutput& output);
};

class SwitchMode {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  AxisMode target_mode);
};

class Homing {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  bool start);
};

}  // namespace cia402

#endif  // CIA402_CIA402_H_
```

## 15. 当前代码状态

当前代码已按本文档迁移为 C++ 主接口：

```text
GetAxisState()
GetAxisHomingState()
PowerAxis::Update()
ClearAxisError::Update()
SwitchMode::Update()
Homing::Update()
```

当前公开头文件为：

```text
include/cia402/cia402.h
```

目前不再提供 C ABI。如果后续需要给 C、Python、C#、LabVIEW 等跨语言调用，建议新增独立文件：

```text
include/cia402/cia402_c.h
```

不要把 C ABI 和 C++ 主接口混在一个头文件里。
