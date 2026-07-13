# CiA402 状态控制动态库工程

这个目录用于实现一个很薄的 C++ CiA 402 状态控制库。它只负责：

- CiA402 模式枚举；
- `0x6041 Statusword` 状态解析；
- `0x6061 Modes of operation display` 模式反馈；
- `0x6040 Controlword` 使能、断使能、清错、回零启动控制；
- `0x6060 Modes of operation` 模式切换输出。

它不负责运动控制，不负责 EtherCAT 主站，也不负责日志打印。

后续交付给使用者时，可以只输出：

- `libcia402.so`
- `include/cia402/cia402.h`
- `README.md`

## 工程结构

```text
CiA402/
  CMakeLists.txt
  NAMING_STYLE.md
  .clang-format
  include/cia402/
    cia402.h
  src/
    cia402.cpp
  examples/
    basic_usage.cpp
```

命名和格式统一参考 [NAMING_STYLE.md](/home/js/ETCAT/CiA402/NAMING_STYLE.md)。

## 编译

```bash
cd /home/js/ETCAT/CiA402
cmake -S . -B build -DCIA402_BUILD_SHARED=ON
cmake --build build
```

## 最小 PDO 数据

这个库只需要 4 个 CiA402 标准对象：

```text
Servo Out / RxPDO
  0x6040 Controlword                    uint16
  0x6060 Modes of operation             int8

Servo In / TxPDO
  0x6041 Statusword                     uint16
  0x6061 Modes of operation display     int8
```

对应结构：

```cpp
struct AxisInput {
  uint16_t statusword = 0;
  int8_t mode_display = 0;
};

struct AxisOutput {
  uint16_t controlword = 0;
  int8_t mode = 0;
};

struct AxisData {
  AxisOutput outData;
  AxisInput inData;
  AxisHomingState homingState = AxisHomingState::kUnknown;
  AxisState axisState = AxisState::kUnknown;
};
```

## 枚举

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

Homing 状态对应关系：

```text
(statusword & 0x3400) == 0x0000 -> kHomingInProgress
(statusword & 0x3400) == 0x0400 -> kHomingInterrupted
(statusword & 0x3400) == 0x1000 -> kHomingAttainedAndNoTarget
(statusword & 0x3400) == 0x1400 -> kHomingFinished
(statusword & 0x3400) == 0x2000 -> kHomingErrorVelocityNotZero
(statusword & 0x3400) == 0x2400 -> kHomingError
```

## 对外函数

这个库不使用类作为 Function Block，只使用 `cia402` namespace 下的函数：

```cpp
const char* Version();

AxisState GetAxisState(AxisData& axis);
AxisHomingState GetAxisHomingState(AxisData& axis);

FbStatus ClearAxisError(AxisData& axis);
FbStatus Homing(AxisData& axis, bool start,
                bool require_operation_enabled = false);
FbStatus PowerAxis(AxisData& axis, bool enable);
FbStatus SwitchMode(AxisData& axis, AxisMode target_mode);
```

所有函数都是单周期一步执行：

```text
输入：axis.inData
输出：axis.outData
状态缓存：axis.axisState / axis.homingState
返回：kDone / kBusy / kError
```

它们不会阻塞，不会 sleep，不会访问 IgH/SOEM，也不会直接读写 PDO 内存。

### Homing 接口边界

`Homing()` 只负责 0x6040 controlword 的 Homing start 位和 0x6041
statusword 的回零状态判断：

- 不负责切换到 Homing 模式；
- 不负责回零结束后切回位置模式；
- 当 0x6061 mode display 不是 Homing 时返回 `kError`；
- 是否要求轴已经 Operation enabled 由 `require_operation_enabled` 入参决定。

推荐流程是上层先调用 `SwitchMode(axis, AxisMode::kHoming)`，确认模式反馈到位后，
再周期调用 `Homing(axis, true, ...)`。

## 使用示例

```cpp
#include "cia402/cia402.h"

cia402::AxisData axis{};
axis.inData.statusword = 0x0040;
axis.inData.mode_display = static_cast<int8_t>(cia402::AxisMode::kHoming);

cia402::GetAxisState(axis);
cia402::PowerAxis(axis, true);
```

## 分层边界

这个库故意不包含：

- `IsFault()` / `IsWarning()` / `IsRemote()` 等辅助判断；
- `ToString()` 或任何日志打印接口；
- `0x607A Target position`;
- `0x60FF Target velocity`;
- `0x6071 Target torque`;
- 编码器单位换算；
- 减速比；
- 轨迹规划；
- 插补；
- IgH/SOEM PDO offset 绑定；
- SDO 参数下发。

这些都属于上层业务、运动控制层、驱动器适配层或主站层。
