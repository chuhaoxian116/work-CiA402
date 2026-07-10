# CiA402 状态控制动态库工程

这个目录用于实现一个很薄、很稳定的 CiA 402 状态控制库。它只负责：

- CiA 402 模式枚举；
- `0x6041 statusword` 状态解析；
- `0x6061 modes of operation display` 模式反馈判断；
- `0x6040 controlword` 使能、断使能、清错、回零启动控制；
- `0x6060 modes of operation` 模式切换输出。

它不负责运动控制，不负责 EtherCAT 主站，也不负责厂商私有参数。

后续交付给使用者时，可以只输出：

- `libcia402.so`
- `include/cia402/cia402.h`
- `include/cia402/cia402.hpp`
- `README.md`

## 工程结构

```text
CiA402/
  CMakeLists.txt
  NAMING_STYLE.md     # 命名与格式规范
  include/cia402/
    cia402.h          # C ABI，对外稳定接口，适合动态库交付
    cia402.hpp        # C++ 轻量封装
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
  0x6040 controlword                  uint16
  0x6060 modes of operation           int8

Servo In / TxPDO
  0x6041 statusword                   uint16
  0x6061 modes of operation display   int8
```

对应数据结构：

```c
typedef struct cia402_axis_input {
    uint16_t statusword;      /* 0x6041 */
    int8_t mode_display;      /* 0x6061 */
} cia402_axis_input_t;

typedef struct cia402_axis_output {
    uint16_t controlword;     /* 0x6040 */
    int8_t mode;              /* 0x6060 */
} cia402_axis_output_t;
```

## 枚举

### AxisMode

```text
CIA402_AXIS_MODE_NONE                    = 0
CIA402_AXIS_MODE_PROFILE_POSITION        = 1
CIA402_AXIS_MODE_VELOCITY                = 2
CIA402_AXIS_MODE_PROFILE_VELOCITY        = 3
CIA402_AXIS_MODE_PROFILE_TORQUE          = 4
CIA402_AXIS_MODE_HOMING                  = 6
CIA402_AXIS_MODE_INTERPOLATED_POSITION   = 7
CIA402_AXIS_MODE_CYCLIC_SYNC_POSITION    = 8
CIA402_AXIS_MODE_CYCLIC_SYNC_VELOCITY    = 9
CIA402_AXIS_MODE_CYCLIC_SYNC_TORQUE      = 10
```

### AxisState

`AxisState` 由 `0x6041 statusword` 解码得到，不需要业务层自己维护：

```text
Not ready to switch on
Switch on disabled
Ready to switch on
Switched on
Operation enabled
Quick stop active
Fault reaction active
Fault
Unknown
```

### AxisHomingState

`AxisHomingState` 由 `0x6061 mode_display` 和 `0x6041 statusword` 解码得到：

```text
Not homing mode
Not started
Running
Attained
Error
Unknown
```

注意：Homing 的参数配置，例如 `0x6098 homing method`、`0x6099 homing speeds`、`0x609A homing acceleration`，不属于这个库。它们应由驱动器适配层或应用配置层通过 SDO 完成。

## 状态查询函数

```c
cia402_axis_state_t cia402_get_axis_state(
    const cia402_axis_input_t *input);

cia402_axis_homing_state_t cia402_get_axis_homing_state(
    const cia402_axis_input_t *input);

int cia402_is_fault(const cia402_axis_input_t *input);
int cia402_is_operation_enabled(const cia402_axis_input_t *input);
int cia402_is_mode_reached(const cia402_axis_input_t *input,
                           cia402_axis_mode_t target_mode);
```

## Function Blocks

所有 function block 都是“单周期一步执行”模型：

```text
输入：本周期 AxisInput
输出：修改 AxisOutput
返回：DONE / BUSY / ERROR
```

它们不会阻塞，不会 sleep，不会访问 IgH/SOEM，也不会直接读写 PDO 内存。调用者应在实时周期里每周期调用一次，然后把 `AxisOutput` 写入真实 PDO。

### PowerAxis

```c
cia402_fb_status_t cia402_power_axis(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output,
    int enable);
```

`enable = 1` 时执行标准使能流程：

```text
Switch on disabled
  -> controlword = 0x0006

Ready to switch on
  -> controlword = 0x0007

Switched on
  -> controlword = 0x000F

Operation enabled
  -> controlword = 0x000F
  -> DONE
```

`enable = 0` 时输出断使能控制字。

### ClearAxisError

```c
cia402_fb_status_t cia402_clear_axis_error(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output);
```

驱动器处于 `Fault` 时输出：

```text
controlword = 0x0080
```

当 fault 消失后返回 `DONE`。

### SwitchMode

```c
cia402_fb_status_t cia402_switch_mode(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output,
    cia402_axis_mode_t target_mode);
```

该函数只做两件事：

```text
output.mode = target_mode
等待 input.mode_display == target_mode
```

是否需要先断使能再切模式、还是可带使能切模式，由调用者或驱动器适配层决定。

### Homing

```c
cia402_fb_status_t cia402_homing(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output,
    int start);
```

这个 Homing 只负责 CiA 402 标准状态控制：

```text
1. output.mode = Homing
2. 等待 mode_display == Homing
3. PowerAxis 进入 Operation enabled
4. controlword bit4 = 1，启动 homing
5. statusword bit12 -> Attained
6. statusword bit13 -> Error
```

回零速度、回零方向、找限位方式、原点偏移等参数不在本库内处理。

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

- `0x607A target position`
- `0x60FF target velocity`
- `0x6071 target torque`
- `0x6064 actual position`
- `0x606C actual velocity`
- `0x6077 actual torque`
- 编码器单位换算
- 减速比
- 轨迹规划
- 插补
- IgH/SOEM PDO offset 绑定
- SDO 参数下发

这些都更适合放到运动控制层或驱动器适配层。
