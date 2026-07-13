#ifndef CIA402_CIA402_H_
#define CIA402_CIA402_H_

#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
#define CIA402_API __attribute__((visibility("default")))
#else
#define CIA402_API
#endif

namespace cia402 {

/**
 * @brief 本库使用的 CiA402 运行模式。
 *
 * 数值对应 0x6060 Modes of operation 和 0x6061 Modes of operation display。
 */
enum class AxisMode : int8_t {
  kHoming = 6,                     // Homing mode，回零模式。
  kCyclicSynchronousPosition = 8,  // CSP，周期同步位置模式。
  kCyclicSynchronousVelocity = 9,  // CSV，周期同步速度模式。
  kCyclicSynchronousTorque = 10    // CST，周期同步转矩模式。
};

/**
 * @brief 由 0x6041 Statusword 解码得到的 CiA402 状态机状态。
 *
 * 无法识别的 Statusword 组合返回 kUnknown。
 */
enum class AxisState : uint8_t {
  kSwitchOnDisabled = 0,     // Switch on disabled，禁止上电状态。
  kReadyToSwitchOn = 1,      // Ready to switch on，准备上电状态。
  kSwitchedOn = 2,           // Switched on，已上电但未使能运行。
  kOperationEnabled = 3,     // Operation enabled，已使能运行。
  kQuickStopActive = 4,      // Quick stop active，快停激活。
  kFaultReactionActive = 5,  // Fault reaction active，故障响应中。
  kFault = 6,                // Fault，故障状态。
  kNotReadyToSwitchOn = 7,   // Not ready to switch on，未准备上电。
  kUnknown = 8               // 未识别状态。
};

/**
 * @brief 由 0x6041 Statusword 中的 Homing 相关位解码得到的回零状态。
 *
 * 回零状态由 Statusword 中的 homing attained、target reached、
 * homing error 相关位组合得到。
 */
enum class AxisHomingState : uint8_t {
  kHomingInProgress = 0,            // 回零进行中。
  kHomingInterrupted = 1,           // 回零被中断。
  kHomingAttainedAndNoTarget = 2,   // 已找到原点，但未到目标位置。
  kHomingFinished = 3,              // 回零完成。
  kHomingErrorVelocityNotZero = 4,  // 回零错误：速度未归零。
  kHomingError = 5,                 // 回零错误。
  kUnknown = 6                      // 未识别回零状态。
};

/**
 * @brief 单周期函数返回值。
 *
 * kDone 表示当前动作完成；kBusy 表示后续周期继续调用；
 * kError 表示当前轴状态不允许继续该动作。
 */
enum class FbStatus : uint8_t {
  kDone = 0,  // 当前动作已经完成。
  kBusy = 1,  // 当前动作正在执行，需要后续周期继续调用。
  kError = 2  // 当前状态不允许执行动作，或动作执行失败。
};

/**
 * @brief 本库需要的最小输入 PDO 数据。
 *
 * 调用前由上层主站把真实 TxPDO 数据拷贝到这里。
 */
struct AxisInput {
  uint16_t statusword = 0;  // 0x6041 Statusword。
  int8_t mode_display = 0;  // 0x6061 Modes of operation display。
};

/**
 * @brief 本库生成的最小输出 PDO 数据。
 *
 * 调用后由上层主站把这里的数据写回真实 RxPDO 区域。
 */
struct AxisOutput {
  uint16_t controlword = 0;  // 0x6040 Controlword。
  int8_t mode = 0;           // 0x6060 Modes of operation。
};

/**
 * @brief 单轴数据对象。
 *
 * inData/outData 对应输入和输出 PDO；axisState/homingState 缓存解码结果。
 */
struct AxisData {
  AxisOutput outData;                                       // 输出 PDO 数据。
  AxisInput inData;                                         // 输入 PDO 数据。
  AxisHomingState homingState = AxisHomingState::kUnknown;  // 缓存的回零状态。
  AxisState axisState = AxisState::kUnknown;                // 缓存的 CiA402 状态。
};

/**
 * @brief 获取库版本号。
 *
 * @return 版本号字符串。
 */
CIA402_API const char* Version();

/**
 * @brief 获取轴的 CiA402 状态。
 *
 * 解码 axis.inData.statusword，更新 axis.axisState 并返回。
 *
 * @param axis 单轴数据对象。
 * @return AxisState 解码后的 CiA402 状态。
 */
CIA402_API AxisState GetAxisState(AxisData& axis);

/**
 * @brief 获取轴的回零状态。
 *
 * 解码 axis.inData.statusword，更新 axis.homingState 并返回。
 *
 * @param axis 单轴数据对象。
 * @return AxisHomingState 解码后的回零状态。
 */
CIA402_API AxisHomingState GetAxisHomingState(AxisData& axis);

/**
 * @brief 清除轴故障。
 *
 * 处于 fault 或 fault reaction active 时输出 fault reset 控制字；
 * 否则清除 fault reset 位并返回完成。
 * 本函数不检测错误是否真的被清除，结果由上层业务判断。
 *
 * @param axis 单轴数据对象。
 * @return FbStatus 单周期执行结果。
 */
CIA402_API FbStatus ClearAxisError(AxisData& axis);

/**
 * @brief 控制 CiA402 Homing 模式。
 *
 * 本函数只负责 Homing start 位和回零状态判断，不负责切换 0x6060
 * 模式，也不负责回零结束后切回位置模式。调用前上层应先通过
 * SwitchMode() 或自己的业务流程切到 Homing 模式。
 *
 * 当 start 为 false 时，本函数清除 Homing start 位并返回 kDone。
 * 当 start 为 true 且 0x6061 反馈不是 Homing 模式时，返回 kError。
 * 如果 require_operation_enabled 为 true，则轴不在 Operation enabled
 * 时也返回 kError；如果为 false，则不检查使能状态。
 *
 * @param axis 单轴数据对象。
 * @param start 是否启动回零。
 * @param require_operation_enabled 是否要求轴已经处于 Operation enabled。
 * @return FbStatus 单周期执行结果。
 */
CIA402_API FbStatus Homing(AxisData& axis, bool start, bool require_operation_enabled = false);

/**
 * @brief 执行标准 CiA402 使能/断使能状态切换。
 *
 * enable 为 true 时按状态字自动计算上使能控制字；
 * enable 为 false 时根据当前状态请求断使能或快停。
 *
 * @param axis 单轴数据对象。
 * @param enable 是否使能。
 * @return FbStatus 单周期执行结果。
 */
CIA402_API FbStatus PowerAxis(AxisData& axis, bool enable);

/**
 * @brief 切换轴运行模式。
 *
 * 写入目标运行模式，并等待 0x6061 反馈相同模式。
 *
 * @param axis 单轴数据对象。
 * @param target_mode 目标运行模式。
 * @return FbStatus 单周期执行结果。
 */
CIA402_API FbStatus SwitchMode(AxisData& axis, AxisMode target_mode);

}  // namespace cia402

#endif
