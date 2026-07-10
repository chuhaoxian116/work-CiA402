# CiA402 状态控制动态库工程

这个目录用于实现一个很薄、很稳定的 C++ CiA 402 状态控制库。它只负责：

- CiA 402 模式枚举；
- `0x6041 Statusword` 状态解析；
- `0x6061 Modes of operation display` 模式反馈判断；
- `0x6040 Controlword` 使能、断使能、清错、回零启动控制；
- `0x6060 Modes of operation` 模式切换输出。

它不负责运动控制，不负责 EtherCAT 主站，也不负责厂商私有参数。

后续交付给使用者时，可以只输出：

- `libcia402.so`
- `include/cia402/cia402.h`
- `README.md`

## 工程结构

```text
CiA402/
  CMakeLists.txt
  NAMING_STYLE.md     # 命名与格式规范
  .clang-format       # Google C++ 格式化配置
  include/cia402/
    cia402.h          # C++ 对外头文件
  src/
    cia402.cpp        # 内部实现，最终不需要交付源码
  examples/
    basic_usage.cpp   # 使用示例
```

命名和格式统一参考 [NAMING_STYLE.md](/home/js/ETCAT/CiA402/NAMING_STYLE.md)。

## 编译

```bash
cd /home/js/ETCAT/CiA402
cmake -S . -B build -DCIA402_BUILD_SHARED=ON
cmake --build build
```

输出动态库：

```text
build/libcia402.so
```

## 最小 PDO 数据

这个库只需要 4 个 CiA 402 标准对象：

```text
Servo Out / RxPDO
  0x6040 Controlword                    uint16
  0x6060 Modes of operation             int8

Servo In / TxPDO
  0x6041 Statusword                     uint16
  0x6061 Modes of operation display     int8
```

对应 C++ 数据结构：

```cpp
namespace cia402 {

struct AxisInput {
  uint16_t statusword;      // 0x6041 Statusword
  int8_t mode_display;      // 0x6061 Modes of operation display
};

struct AxisOutput {
  uint16_t controlword;     // 0x6040 Controlword
  int8_t mode;              // 0x6060 Modes of operation
};

}  // namespace cia402
```

## 枚举

### AxisMode

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

### AxisState

`AxisState` 由 `0x6041 Statusword` 解码得到，不需要业务层自己维护：

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

### AxisHomingState

`AxisHomingState` 由 `0x6061 mode_display` 和 `0x6041 statusword` 解码得到：

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

注意：Homing 的参数配置，例如 `0x6098 Homing method`、`0x6099 Homing speeds`、`0x609A Homing acceleration`，不属于这个库。它们应由驱动器适配层或应用配置层通过 SDO 完成。

## 状态查询函数

```cpp
AxisState GetAxisState(const AxisInput& input);
AxisHomingState GetAxisHomingState(const AxisInput& input);

bool IsFault(const AxisInput& input);
bool IsOperationEnabled(const AxisInput& input);
bool IsModeReached(const AxisInput& input, AxisMode target_mode);

const char* ToString(AxisState state);
const char* ToString(AxisHomingState state);
const char* ToString(AxisMode mode);
```

## Function Blocks

所有 Function Block 都是“单周期一步执行”模型：

```text
输入：本周期 AxisInput
输出：修改 AxisOutput
返回：kDone / kBusy / kError
```

它们不会阻塞，不会 sleep，不会访问 IgH/SOEM，也不会直接读写 PDO 内存。调用者应在实时周期里每周期调用一次，然后把 `AxisOutput` 写入真实 PDO。

### PowerAxis

```cpp
class PowerAxis {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  bool enable);
};
```

`enable = true` 时执行标准使能流程：

```text
Switch on disabled
  -> controlword = 0x0006

Ready to switch on
  -> controlword = 0x0007

Switched on
  -> controlword = 0x000F

Operation enabled
  -> controlword = 0x000F
  -> kDone
```

`enable = false` 时输出断使能控制字。

### ClearAxisError

```cpp
class ClearAxisError {
 public:
  FbStatus Update(const AxisInput& input, AxisOutput& output);
};
```

驱动器处于 `Fault` 时输出：

```text
controlword = 0x0080
```

当 fault 消失后返回 `kDone`。

### SwitchMode

```cpp
class SwitchMode {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  AxisMode target_mode);
};
```

该功能块只做两件事：

```text
output.mode = target_mode
等待 input.mode_display == target_mode
```

是否需要先断使能再切模式、还是可带使能切模式，由调用者或驱动器适配层决定。

### Homing

```cpp
class Homing {
 public:
  FbStatus Update(const AxisInput& input,
                  AxisOutput& output,
                  bool start);
};
```

这个 Homing 只负责 CiA 402 标准状态控制：

```text
1. output.mode = AxisMode::kHoming
2. 等待 mode_display == Homing
3. PowerAxis 进入 Operation enabled
4. controlword bit4 = 1，启动 homing
5. statusword bit12 -> Attained
6. statusword bit13 -> Error
```

回零速度、回零方向、找限位方式、原点偏移等参数不在本库内处理。

## 使用示例

```cpp
#include "cia402/cia402.h"

cia402::AxisInput input{};
input.statusword = 0x0040;
input.mode_display = static_cast<int8_t>(cia402::AxisMode::kNone);

cia402::AxisOutput output{};

cia402::PowerAxis power_axis;
cia402::FbStatus status = power_axis.Update(input, output, true);
```

## 分层边界

建议保持如下边界：

```text
应用层 / 机器人层
  机器人业务、动作流程、安全策略

运动控制层
  轨迹规划、插补、软限位、单位换算、目标位置/速度/力矩

驱动器适配层
  厂商私有对象、SDO 参数、PDO 映射、绝对值编码器、特殊报警

CiA402 状态控制库
  状态解析、模式切换、使能、清错、Homing 启停

EtherCAT 主站层
  IgH / SOEM、domain、PDO 收发、DC、网卡
```

这个库故意不包含：

- `0x607A Target position`
- `0x60FF Target velocity`
- `0x6071 Target torque`
- `0x6064 Position actual value`
- `0x606C Velocity actual value`
- `0x6077 Torque actual value`
- 编码器单位换算
- 减速比
- 轨迹规划
- 插补
- IgH/SOEM PDO offset 绑定
- SDO 参数下发
