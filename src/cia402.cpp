#include "cia402/cia402.h"

namespace cia402 {
namespace {

/**
 * @brief 0x6040 Controlword 内部常量。
 */
namespace controlword {

constexpr uint16_t kDisableVoltage = 0x0000;
constexpr uint16_t kShutdown = 0x0006;
constexpr uint16_t kSwitchOn = 0x0007;
constexpr uint16_t kEnableOperation = 0x000F;
constexpr uint16_t kDisableOperation = 0x0007;
constexpr uint16_t kFaultReset = 0x0080;
constexpr uint16_t kHomingStartBit = 0x0010;

}

/**
 * @brief 0x6041 Statusword 回零状态解码常量。
 */
namespace homing_status {

constexpr uint16_t kMask = 0x3400;
constexpr uint16_t kInProgress = 0x0000;
constexpr uint16_t kInterrupted = 0x0400;
constexpr uint16_t kAttainedAndNoTarget = 0x1000;
constexpr uint16_t kFinished = 0x1400;
constexpr uint16_t kErrorVelocityNotZero = 0x2000;
constexpr uint16_t kError = 0x2400;

}

/**
 * @brief 清除 homing start 位，保留 controlword 其他位。
 *
 * @param controlword 当前控制字。
 * @return uint16_t 清除 homing start 位后的控制字。
 */
uint16_t ClearHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword &
                               ~controlword::kHomingStartBit);
}

/**
 * @brief 置位 homing start 位，保留 controlword 其他位。
 *
 * @param controlword 当前控制字。
 * @return uint16_t 置位 homing start 位后的控制字。
 */
uint16_t SetHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword | controlword::kHomingStartBit);
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
  switch (statusword & homing_status::kMask) {
    case homing_status::kInProgress:
      return AxisHomingState::kHomingInProgress;
    case homing_status::kInterrupted:
      return AxisHomingState::kHomingInterrupted;
    case homing_status::kAttainedAndNoTarget:
      return AxisHomingState::kHomingAttainedAndNoTarget;
    case homing_status::kFinished:
      return AxisHomingState::kHomingFinished;
    case homing_status::kErrorVelocityNotZero:
      return AxisHomingState::kHomingErrorVelocityNotZero;
    case homing_status::kError:
      return AxisHomingState::kHomingError;
  }

  return AxisHomingState::kUnknown;
}

}

const char* Version() { return "0.5.0"; }

AxisState GetAxisState(AxisData& axis) {
  axis.axisState = DecodeAxisState(axis.inData.statusword);
  return axis.axisState;
}

AxisHomingState GetAxisHomingState(AxisData& axis) {
  axis.homingState = DecodeAxisHomingState(axis.inData.statusword);
  return axis.homingState;
}

FbStatus ClearAxisError(AxisData& axis) {
  const AxisState state = GetAxisState(axis);

  if (state == AxisState::kFault) {
    axis.outData.controlword = controlword::kFaultReset;
    return FbStatus::kBusy;
  }

  if (state == AxisState::kFaultReactionActive) {
    axis.outData.controlword = controlword::kDisableVoltage;
    return FbStatus::kBusy;
  }

  axis.outData.controlword = controlword::kDisableVoltage;
  return FbStatus::kDone;
}

FbStatus Homing(AxisData& axis, bool start) {
  axis.outData.mode = static_cast<int8_t>(AxisMode::kHoming);

  if (!start) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kDone;
  }

  const AxisHomingState homing_state = GetAxisHomingState(axis);

  if (homing_state == AxisHomingState::kHomingError ||
      homing_state == AxisHomingState::kHomingErrorVelocityNotZero) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kError;
  }

  if (homing_state == AxisHomingState::kHomingFinished) {
    axis.outData.controlword =
        ClearHomingStart(controlword::kEnableOperation);
    return FbStatus::kDone;
  }

  if (axis.inData.mode_display != static_cast<int8_t>(AxisMode::kHoming)) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kBusy;
  }

  const FbStatus power_status = PowerAxis(axis, true);
  if (power_status == FbStatus::kError) {
    return FbStatus::kError;
  }
  if (power_status == FbStatus::kBusy) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kBusy;
  }

  axis.outData.controlword = SetHomingStart(axis.outData.controlword);
  return FbStatus::kBusy;
}

FbStatus PowerAxis(AxisData& axis, bool enable) {
  const AxisState state = GetAxisState(axis);

  if (!enable) {
    axis.outData.controlword = controlword::kDisableOperation;
    return state == AxisState::kOperationEnabled ? FbStatus::kBusy
                                                 : FbStatus::kDone;
  }

  if (state == AxisState::kFault || state == AxisState::kFaultReactionActive) {
    return FbStatus::kError;
  }

  switch (state) {
    case AxisState::kSwitchOnDisabled:
      axis.outData.controlword = controlword::kShutdown;
      return FbStatus::kBusy;
    case AxisState::kReadyToSwitchOn:
      axis.outData.controlword = controlword::kSwitchOn;
      return FbStatus::kBusy;
    case AxisState::kSwitchedOn:
      axis.outData.controlword = controlword::kEnableOperation;
      return FbStatus::kBusy;
    case AxisState::kOperationEnabled:
      axis.outData.controlword = controlword::kEnableOperation;
      return FbStatus::kDone;
    case AxisState::kQuickStopActive:
      axis.outData.controlword = controlword::kDisableOperation;
      return FbStatus::kBusy;
    case AxisState::kFaultReactionActive:
    case AxisState::kFault:
    case AxisState::kNotReadyToSwitchOn:
    case AxisState::kUnknown:
      axis.outData.controlword = controlword::kDisableVoltage;
      return FbStatus::kBusy;
  }

  axis.outData.controlword = controlword::kDisableVoltage;
  return FbStatus::kBusy;
}

FbStatus SwitchMode(AxisData& axis, AxisMode target_mode) {
  axis.outData.mode = static_cast<int8_t>(target_mode);

  if (axis.inData.mode_display == static_cast<int8_t>(target_mode)) {
    return FbStatus::kDone;
  }
  return FbStatus::kBusy;
}

}
