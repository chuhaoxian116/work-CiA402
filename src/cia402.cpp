#include "cia402/cia402.h"

namespace cia402 {
namespace {

/**
 * @brief Homing 模式下，0x6040 bit4 用作 homing start。
 */
constexpr uint16_t kControlwordHomingStartBit = 0x0010;

/**
 * @brief 回零状态解码规则与当前 PLC 参考实现一致：statusword & 0x3400。
 */
constexpr uint16_t kStatuswordHomingMask = 0x3400;
constexpr uint16_t kStatuswordHomingInProgress = 0x0000;
constexpr uint16_t kStatuswordHomingInterrupted = 0x0400;
constexpr uint16_t kStatuswordHomingAttainedAndNoTarget = 0x1000;
constexpr uint16_t kStatuswordHomingFinished = 0x1400;
constexpr uint16_t kStatuswordHomingErrorVelocityNotZero = 0x2000;
constexpr uint16_t kStatuswordHomingError = 0x2400;

/**
 * @brief 状态机内部使用的标准 CiA402 控制字。
 */
constexpr uint16_t kControlwordDisableVoltage = 0x0000;
constexpr uint16_t kControlwordShutdown = 0x0006;
constexpr uint16_t kControlwordSwitchOn = 0x0007;
constexpr uint16_t kControlwordEnableOperation = 0x000F;
constexpr uint16_t kControlwordDisableOperation = 0x0007;
constexpr uint16_t kControlwordFaultReset = 0x0080;

/**
 * @brief 清除 homing start 位，保留 controlword 其他位。
 *
 * @param controlword 当前控制字。
 * @return uint16_t 清除 homing start 位后的控制字。
 */
uint16_t ClearHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword & ~kControlwordHomingStartBit);
}

/**
 * @brief 置位 homing start 位，保留 controlword 其他位。
 *
 * @param controlword 当前控制字。
 * @return uint16_t 置位 homing start 位后的控制字。
 */
uint16_t SetHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword | kControlwordHomingStartBit);
}

/**
 * @brief 从 Statusword 解码 CiA402 状态机状态。
 *
 * 掩码和比较值来自 CiA402 标准状态字模式。
 *
 * @param statusword 0x6041 Statusword。
 * @return AxisState 解码后的 CiA402 状态。
 */
AxisState DecodeAxisState(uint16_t statusword) {
  const uint16_t masked = statusword & 0x006F;

  if ((statusword & 0x004F) == 0x0040) {
    return AxisState::kSwitchOnDisabled;
  }
  if (masked == 0x0021) {
    return AxisState::kReadyToSwitchOn;
  }
  if (masked == 0x0023) {
    return AxisState::kSwitchedOn;
  }
  if (masked == 0x0027) {
    return AxisState::kOperationEnabled;
  }
  if (masked == 0x0007) {
    return AxisState::kQuickStopActive;
  }
  if ((statusword & 0x004F) == 0x000F) {
    return AxisState::kFaultReactionActive;
  }
  if ((statusword & 0x004F) == 0x0008) {
    return AxisState::kFault;
  }
  if ((statusword & 0x004F) == 0x0000) {
    return AxisState::kNotReadyToSwitchOn;
  }

  return AxisState::kUnknown;
}

/**
 * @brief 从 Statusword 解码回零状态。
 *
 * 这里故意不检查 mode_display，以保持和 PLC 参考函数一致。
 *
 * @param statusword 0x6041 Statusword。
 * @return AxisHomingState 解码后的回零状态。
 */
AxisHomingState DecodeAxisHomingState(uint16_t statusword) {
  switch (statusword & kStatuswordHomingMask) {
    case kStatuswordHomingInProgress:
      return AxisHomingState::kHomingInProgress;
    case kStatuswordHomingInterrupted:
      return AxisHomingState::kHomingInterrupted;
    case kStatuswordHomingAttainedAndNoTarget:
      return AxisHomingState::kHomingAttainedAndNoTarget;
    case kStatuswordHomingFinished:
      return AxisHomingState::kHomingFinished;
    case kStatuswordHomingErrorVelocityNotZero:
      return AxisHomingState::kHomingErrorVelocityNotZero;
    case kStatuswordHomingError:
      return AxisHomingState::kHomingError;
  }

  return AxisHomingState::kUnknown;
}

}

const char* Version() { return "0.5.0"; }

AxisState GetAxisState(AxisData& axis) {
  /**
   * @brief 缓存解码结果，避免上层重复解析同一个 statusword。
   */
  axis.axisState = DecodeAxisState(axis.inData.statusword);
  return axis.axisState;
}

AxisHomingState GetAxisHomingState(AxisData& axis) {
  /**
   * @brief 缓存回零状态，便于上层直接读取 axis.homingState。
   */
  axis.homingState = DecodeAxisHomingState(axis.inData.statusword);
  return axis.homingState;
}

FbStatus ClearAxisError(AxisData& axis) {
  const AxisState state = GetAxisState(axis);

  /**
   * @brief Fault 状态下输出 fault reset，等待驱动器离开 Fault。
   */
  if (state == AxisState::kFault) {
    axis.outData.controlword = kControlwordFaultReset;
    return FbStatus::kBusy;
  }

  /**
   * @brief Fault reaction active 还不是可复位状态，先保持断电压。
   */
  if (state == AxisState::kFaultReactionActive) {
    axis.outData.controlword = kControlwordDisableVoltage;
    return FbStatus::kBusy;
  }

  /**
   * @brief 已不在故障状态。
   */
  axis.outData.controlword = kControlwordDisableVoltage;
  return FbStatus::kDone;
}

FbStatus Homing(AxisData& axis, bool start) {
  /**
   * @brief Homing 函数始终请求 0x6060 = Homing。
   */
  axis.outData.mode = static_cast<int8_t>(AxisMode::kHoming);

  if (!start) {
    /**
     * @brief 停止请求只清除 homing start 位。
     */
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kDone;
  }

  const AxisHomingState homing_state = GetAxisHomingState(axis);

  /**
   * @brief 回零错误直接返回 kError，具体错误日志交给上层。
   */
  if (homing_state == AxisHomingState::kHomingError ||
      homing_state == AxisHomingState::kHomingErrorVelocityNotZero) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kError;
  }

  /**
   * @brief PLC 参考实现中，0x1400 表示回零完成。
   */
  if (homing_state == AxisHomingState::kHomingFinished) {
    axis.outData.controlword = ClearHomingStart(kControlwordEnableOperation);
    return FbStatus::kDone;
  }

  /**
   * @brief 等待驱动器通过 0x6061 确认已经进入 Homing 模式。
   */
  if (axis.inData.mode_display != static_cast<int8_t>(AxisMode::kHoming)) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kBusy;
  }

  /**
   * @brief 回零启动前必须先使能到 Operation enabled。
   */
  const FbStatus power_status = PowerAxis(axis, true);
  if (power_status == FbStatus::kError) {
    return FbStatus::kError;
  }
  if (power_status == FbStatus::kBusy) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kBusy;
  }

  /**
   * @brief 轴已使能且处于 Homing 模式，置位 homing start。
   */
  axis.outData.controlword = SetHomingStart(axis.outData.controlword);
  return FbStatus::kBusy;
}

FbStatus PowerAxis(AxisData& axis, bool enable) {
  const AxisState state = GetAxisState(axis);

  if (!enable) {
    /**
     * @brief 请求断使能，直到驱动器不再反馈 Operation enabled。
     */
    axis.outData.controlword = kControlwordDisableOperation;
    return state == AxisState::kOperationEnabled ? FbStatus::kBusy
                                                 : FbStatus::kDone;
  }

  /**
   * @brief 故障状态不直接使能，必须由上层先调用 ClearAxisError()。
   */
  if (state == AxisState::kFault || state == AxisState::kFaultReactionActive) {
    return FbStatus::kError;
  }

  /**
   * @brief 标准 CiA402 使能序列。
   */
  switch (state) {
    case AxisState::kSwitchOnDisabled:
      axis.outData.controlword = kControlwordShutdown;
      return FbStatus::kBusy;
    case AxisState::kReadyToSwitchOn:
      axis.outData.controlword = kControlwordSwitchOn;
      return FbStatus::kBusy;
    case AxisState::kSwitchedOn:
      axis.outData.controlword = kControlwordEnableOperation;
      return FbStatus::kBusy;
    case AxisState::kOperationEnabled:
      axis.outData.controlword = kControlwordEnableOperation;
      return FbStatus::kDone;
    case AxisState::kQuickStopActive:
      axis.outData.controlword = kControlwordDisableOperation;
      return FbStatus::kBusy;
    case AxisState::kFaultReactionActive:
    case AxisState::kFault:
    case AxisState::kNotReadyToSwitchOn:
    case AxisState::kUnknown:
      axis.outData.controlword = kControlwordDisableVoltage;
      return FbStatus::kBusy;
  }

  /**
   * @brief 理论不可达，保守输出断电压。
   */
  axis.outData.controlword = kControlwordDisableVoltage;
  return FbStatus::kBusy;
}

FbStatus SwitchMode(AxisData& axis, AxisMode target_mode) {
  /**
   * @brief 这里只请求目标模式并等待 mode_display 到位。
   *
   * 是否需要先断使能由上层或厂商适配层决定。
   */
  axis.outData.mode = static_cast<int8_t>(target_mode);

  if (axis.inData.mode_display == static_cast<int8_t>(target_mode)) {
    return FbStatus::kDone;
  }
  return FbStatus::kBusy;
}

}
