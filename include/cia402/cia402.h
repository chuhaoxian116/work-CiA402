#ifndef CIA402_CIA402_H_
#define CIA402_CIA402_H_

#include <cstdint>

namespace cia402 {

/**
 * @brief 本库使用的 CiA402 运行模式。
 *
 * 数值对应 0x6060 Modes of operation 和 0x6061 Modes of operation display。
 */
enum class AxisMode : int8_t {
  kHoming = 6,
  kCyclicSynchronousPosition = 8,
  kCyclicSynchronousVelocity = 9,
  kCyclicSynchronousTorque = 10
};

/**
 * @brief 由 0x6041 Statusword 解码得到的 CiA402 状态机状态。
 *
 * 枚举顺序参考当前 PLC 库；无法识别时返回 kUnknown。
 */
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

/**
 * @brief 由 0x6041 Statusword 中的 Homing 相关位解码得到的回零状态。
 *
 * 当前规则与 PLC 参考实现一致：statusword & 0x3400。
 */
enum class AxisHomingState {
  kHomingInProgress,
  kHomingInterrupted,
  kHomingAttainedAndNoTarget,
  kHomingFinished,
  kHomingErrorVelocityNotZero,
  kHomingError,
  kUnknown
};

/**
 * @brief 单周期函数返回值。
 *
 * kDone 表示当前动作完成；kBusy 表示后续周期继续调用；
 * kError 表示当前轴状态不允许继续该动作。
 */
enum class FbStatus { kDone, kBusy, kError };

/**
 * @brief 本库需要的最小输入 PDO 数据。
 *
 * 调用前由上层主站把真实 TxPDO 数据拷贝到这里。
 */
struct AxisInput {
  uint16_t statusword = 0;  ///< 0x6041 Statusword。
  int8_t mode_display = 0;  ///< 0x6061 Modes of operation display。
};

/**
 * @brief 本库生成的最小输出 PDO 数据。
 *
 * 调用后由上层主站把这里的数据写回真实 RxPDO 区域。
 */
struct AxisOutput {
  uint16_t controlword = 0;  ///< 0x6040 Controlword。
  int8_t mode = 0;           ///< 0x6060 Modes of operation。
};

/**
 * @brief 单轴数据对象。
 *
 * inData/outData 对应输入和输出 PDO；axisState/homingState 缓存解码结果。
 */
struct AxisData {
  AxisOutput outData;  ///< 输出 PDO 数据。
  AxisInput inData;    ///< 输入 PDO 数据。
  AxisHomingState homingState =
      AxisHomingState::kUnknown;  ///< 缓存的回零状态。
  AxisState axisState = AxisState::kUnknown;  ///< 缓存的 CiA402 状态。
};

/**
 * @brief 获取库版本号。
 *
 * @return 版本号字符串。
 */
const char* Version();

/**
 * @brief 获取轴的 CiA402 状态。
 *
 * 解码 axis.inData.statusword，更新 axis.axisState 并返回。
 *
 * @param axis 单轴数据对象。
 * @return AxisState 解码后的 CiA402 状态。
 */
AxisState GetAxisState(AxisData& axis);

/**
 * @brief 获取轴的回零状态。
 *
 * 解码 axis.inData.statusword，更新 axis.homingState 并返回。
 *
 * @param axis 单轴数据对象。
 * @return AxisHomingState 解码后的回零状态。
 */
AxisHomingState GetAxisHomingState(AxisData& axis);

/**
 * @brief 清除轴故障。
 *
 * 只负责输出标准 CiA402 fault reset 控制字；
 * 厂商错误码读取和日志打印由上层业务处理。
 *
 * @param axis 单轴数据对象。
 * @return FbStatus 单周期执行结果。
 */
FbStatus ClearAxisError(AxisData& axis);

/**
 * @brief 控制 CiA402 Homing 模式。
 *
 * 回零方式、速度、加速度、偏移等参数由上层通过 SDO 或厂商适配层配置。
 *
 * @param axis 单轴数据对象。
 * @param start 是否启动回零。
 * @return FbStatus 单周期执行结果。
 */
FbStatus Homing(AxisData& axis, bool start);

/**
 * @brief 执行标准 CiA402 使能/断使能状态切换。
 *
 * enable 为 true 时尝试进入 Operation enabled；否则请求断使能。
 *
 * @param axis 单轴数据对象。
 * @param enable 是否使能。
 * @return FbStatus 单周期执行结果。
 */
FbStatus PowerAxis(AxisData& axis, bool enable);

/**
 * @brief 切换轴运行模式。
 *
 * 写入目标运行模式，并等待 0x6061 反馈相同模式。
 *
 * @param axis 单轴数据对象。
 * @param target_mode 目标运行模式。
 * @return FbStatus 单周期执行结果。
 */
FbStatus SwitchMode(AxisData& axis, AxisMode target_mode);

}

#endif
