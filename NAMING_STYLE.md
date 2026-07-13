# CiA402 命名与格式规范

本工程采用接近 Google C++ Style 的命名规则，但 `AxisData` 内部字段按当前接口约定保留 `outData / inData / homingState / axisState`。

## 基本规则

```text
类型名：PascalCase
函数名：PascalCase
普通变量 / 函数参数：snake_case
结构体成员：当前接口按约定使用 outData / inData / homingState / axisState
常量：kPascalCase
枚举值：kPascalCase
命名空间：snake_case
文件名：lower_snake_case
缩进：2 空格
```

## 工程边界

这个库只负责：

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
轨迹规划
日志打印
IgH / SOEM PDO offset
SDO 参数下发
厂商私有对象
```

## 对外接口风格

所有接口放在 `cia402` namespace 下：

```cpp
namespace cia402 {

AxisState GetAxisState(AxisData& axis);
AxisHomingState GetAxisHomingState(AxisData& axis);

FbStatus ClearAxisError(AxisData& axis);
FbStatus Homing(AxisData& axis, bool start);
FbStatus PowerAxis(AxisData& axis, bool enable);
FbStatus SwitchMode(AxisData& axis, AxisMode target_mode);

}  // namespace cia402
```

不要使用类封装 Function Block：

```cpp
class PowerAxis;       // 不采用
PowerAxis::Update();   // 不采用
```

也不要提供显示层接口：

```cpp
ToString();            // 不采用
日志打印函数;          // 不采用
```

## 枚举

枚举类型使用 PascalCase，枚举值使用 `kPascalCase`。

```cpp
enum class AxisMode : int8_t {
  kHoming = 6,
  kCyclicSynchronousPosition = 8,
  kCyclicSynchronousVelocity = 9,
  kCyclicSynchronousTorque = 10
};
```

```cpp
enum class AxisState {
  kSwitchOnDisabled,
  kReadyToSwitchOn,
  kSwitchedOn,
  kOperationEnabled,
  kQuickStopActive,
  kFaultReactionActive,
  kFault,
  kNotReadyToSwitchOn,
  kUnknown
};
```

```cpp
enum class AxisHomingState {
  kHomingInProgress,
  kHomingInterrupted,
  kHomingAttainedAndNoTarget,
  kHomingFinished,
  kHomingErrorVelocityNotZero,
  kHomingError,
  kUnknown
};
```

## 数据结构

```cpp
struct AxisInput {
  uint16_t statusword = 0;  // 0x6041 Statusword
  int8_t mode_display = 0;  // 0x6061 Modes of operation display
};

struct AxisOutput {
  uint16_t controlword = 0;  // 0x6040 Controlword
  int8_t mode = 0;           // 0x6060 Modes of operation
};

struct AxisData {
  AxisOutput outData;
  AxisInput inData;
  AxisHomingState homingState = AxisHomingState::kUnknown;
  AxisState axisState = AxisState::kUnknown;
};
```

## 格式

本项目使用 `.clang-format`：

```yaml
BasedOnStyle: Google
Language: Cpp
```

如果环境安装了 `clang-format`，可以执行：

```bash
cd /home/js/ETCAT/CiA402
clang-format -i include/cia402/cia402.h src/cia402.cpp examples/basic_usage.cpp
```
