#include "cia402/cia402.h"

namespace cia402 {
namespace {

// 0x6040 Controlword 内部常量。
namespace controlword {

constexpr uint16_t kDisableVoltage = 0x0000;  // 关闭电压输出。
constexpr uint16_t kFaultReset = 0x0080;      // 故障复位控制字。

// 清除 switch on 和 fault reset 相关位，保留 enable voltage/quick stop。
constexpr uint16_t kClearSwitchOnAndFaultResetMask = 0x007E;

// 清除 enable operation 和 fault reset 相关位，保留上电所需基础位。
constexpr uint16_t kClearEnableOperationAndFaultResetMask = 0x0077;

constexpr uint16_t kClearFaultResetMask = 0x007F;    // 清除 fault reset 位。
constexpr uint16_t kClearQuickStopBitMask = 0x00FB;  // 清除 quick stop 位。
constexpr uint16_t kSwitchOnBit = 0x0001;            // bit0: switch on。
constexpr uint16_t kEnableVoltageBit = 0x0002;       // bit1: enable voltage。
constexpr uint16_t kQuickStopBit = 0x0004;           // bit2: quick stop。
constexpr uint16_t kEnableOperationBit = 0x0008;     // bit3: enable operation。
constexpr uint16_t kHomingStartBit = 0x0010;         // bit4: homing start。
constexpr uint16_t kFaultResetBit = 0x0080;          // bit7: fault reset。

}

// 0x6041 Statusword 回零状态解码常量。
namespace homing_status {

constexpr uint16_t kMask = 0x3400;                  // 回零状态解码掩码。
constexpr uint16_t kInProgress = 0x0000;            // 回零进行中。
constexpr uint16_t kInterrupted = 0x0400;           // 回零被中断。
constexpr uint16_t kAttainedAndNoTarget = 0x1000;   // 已找到原点，未到目标。
constexpr uint16_t kFinished = 0x1400;              // 回零完成。
constexpr uint16_t kErrorVelocityNotZero = 0x2000;  // 回零错误：速度未归零。
constexpr uint16_t kError = 0x2400;                 // 回零错误。

}

// 清除 homing start 位，保留 controlword 其他位。
uint16_t ClearHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword &
                               ~controlword::kHomingStartBit);
}

// 置位 homing start 位，保留 controlword 其他位。
uint16_t SetHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword | controlword::kHomingStartBit);
}

// 清除 fault reset 位。
uint16_t ClearFaultReset(uint16_t controlword) {
  return static_cast<uint16_t>(controlword & ~controlword::kFaultResetBit);
}

// 从 Statusword 解码 CiA402 状态机状态。
AxisState DecodeAxisState(uint16_t statusword) {
  // CiA402 状态机主要由 bit0/1/2/3/5/6 组合判断。
  const uint16_t masked = statusword & 0x006F;

  // 不同状态的掩码不同，因此不能只用同一个 masked 变量比较全部状态。
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

// 从 Statusword 解码回零状态。
// 这里只处理 Statusword 位组合，是否处于 Homing 模式由 Homing() 判断。
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

const char* Version() { return "0.6.0"; }

AxisState GetAxisState(AxisData& axis) {
  // 缓存本周期状态解析结果，便于上层或后续函数复用。
  axis.axisState = DecodeAxisState(axis.inData.statusword);
  return axis.axisState;
}

AxisHomingState GetAxisHomingState(AxisData& axis) {
  // 缓存本周期回零状态解析结果。
  axis.homingState = DecodeAxisHomingState(axis.inData.statusword);
  return axis.homingState;
}

FbStatus ClearAxisError(AxisData& axis) {
  // 先基于最新 Statusword 刷新轴状态。
  const AxisState state = GetAxisState(axis);

  // 只有故障态和故障响应态需要持续输出 fault reset。
  if (state == AxisState::kFault) {
    axis.outData.controlword = controlword::kFaultReset;
    return FbStatus::kBusy;
  }

  if (state == AxisState::kFaultReactionActive) {
    axis.outData.controlword = controlword::kFaultReset;
    return FbStatus::kBusy;
  }

  // 非故障态清除 fault reset 位，避免复位位长期保持。
  axis.outData.controlword = ClearFaultReset(axis.outData.controlword);
  return FbStatus::kDone;
}

FbStatus Homing(AxisData& axis, bool start, bool require_operation_enabled) {
  // start 为 false 时只撤销 homing start 位，不改变运行模式。
  if (!start) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kDone;
  }

  // 是否必须在 Operation enabled 状态下回零由调用者决定。
  if (require_operation_enabled &&
      GetAxisState(axis) != AxisState::kOperationEnabled) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kError;
  }

  // Homing() 不负责切模式；模式未到位时直接返回错误。
  if (axis.inData.mode_display != static_cast<int8_t>(AxisMode::kHoming)) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kError;
  }

  // 模式确认后置位 homing start，开始或保持回零动作。
  axis.outData.controlword = SetHomingStart(axis.outData.controlword);

  const AxisHomingState homing_state = GetAxisHomingState(axis);

  // 回零异常或被中断时撤销 start 位，由上层决定后续处理。
  if (homing_state == AxisHomingState::kHomingError ||
      homing_state == AxisHomingState::kHomingInterrupted ||
      homing_state == AxisHomingState::kHomingAttainedAndNoTarget ||
      homing_state == AxisHomingState::kHomingErrorVelocityNotZero) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kError;
  }

  // 回零完成后撤销 start 位，但不自动切回其他运行模式。
  if (homing_state == AxisHomingState::kHomingFinished) {
    axis.outData.controlword = ClearHomingStart(axis.outData.controlword);
    return FbStatus::kDone;
  }

  // 其他可识别或暂未识别状态均保持 Busy，等待后续周期继续判断。
  return FbStatus::kBusy;
}

FbStatus PowerAxis(AxisData& axis, bool enable) {
  // 每次调用都基于最新 Statusword 决定下一步控制字。
  const AxisState state = GetAxisState(axis);

  if (!enable) {
    // 断使能分支：根据当前状态请求快停、关闭电压或保持等待。
    switch (state) {
      case AxisState::kOperationEnabled:
        // Operation enabled 下撤销 quick stop 位，进入快停/停机路径。
        axis.outData.controlword = static_cast<uint16_t>(
            axis.outData.controlword & controlword::kClearQuickStopBitMask);
        axis.outData.controlword = static_cast<uint16_t>(
            axis.outData.controlword | controlword::kEnableVoltageBit);
        return FbStatus::kBusy;
      case AxisState::kSwitchOnDisabled:
        // 已经进入禁止上电状态后，清空控制字并认为断使能完成。
        axis.outData.controlword = controlword::kDisableVoltage;
        return FbStatus::kDone;
      case AxisState::kFault:
      case AxisState::kFaultReactionActive:
        // 故障相关状态下不强行清零，只保持 enable voltage 位等待状态变化。
        axis.outData.controlword = static_cast<uint16_t>(
            axis.outData.controlword | controlword::kEnableVoltageBit);
        return FbStatus::kBusy;
      case AxisState::kReadyToSwitchOn:
      case AxisState::kSwitchedOn:
      case AxisState::kQuickStopActive:
      case AxisState::kNotReadyToSwitchOn:
      case AxisState::kUnknown:
        return FbStatus::kBusy;
    }
  }

  // 上使能分支：按 CiA402 状态机逐步生成控制字。
  switch (state) {
    case AxisState::kNotReadyToSwitchOn:
    case AxisState::kSwitchOnDisabled:
      // 请求 Shutdown：enable voltage + quick stop。
      axis.outData.controlword = static_cast<uint16_t>(
          axis.outData.controlword &
          controlword::kClearSwitchOnAndFaultResetMask);
      axis.outData.controlword = static_cast<uint16_t>(
          axis.outData.controlword | controlword::kEnableVoltageBit |
          controlword::kQuickStopBit);
      return FbStatus::kBusy;
    case AxisState::kReadyToSwitchOn:
      // 请求 Switch on：补齐 switch on 位。
      axis.outData.controlword = static_cast<uint16_t>(
          axis.outData.controlword &
          controlword::kClearEnableOperationAndFaultResetMask);
      axis.outData.controlword = static_cast<uint16_t>(
          axis.outData.controlword | controlword::kSwitchOnBit |
          controlword::kEnableVoltageBit | controlword::kQuickStopBit);
      return FbStatus::kBusy;
    case AxisState::kSwitchedOn:
      // 请求 Enable operation：补齐 enable operation 位。
      axis.outData.controlword = static_cast<uint16_t>(
          axis.outData.controlword & controlword::kClearFaultResetMask);
      axis.outData.controlword = static_cast<uint16_t>(
          axis.outData.controlword | controlword::kSwitchOnBit |
          controlword::kEnableVoltageBit | controlword::kQuickStopBit |
          controlword::kEnableOperationBit);
      return FbStatus::kBusy;
    case AxisState::kOperationEnabled:
      // 已经使能完成，不再改写控制字。
      return FbStatus::kDone;
    case AxisState::kFaultReactionActive:
    case AxisState::kFault:
      // 上使能接口不自动清错，故障处理交给 ClearAxisError()。
      return FbStatus::kError;
    case AxisState::kQuickStopActive:
    case AxisState::kUnknown:
      return FbStatus::kBusy;
  }

  return FbStatus::kBusy;
}

FbStatus SwitchMode(AxisData& axis, AxisMode target_mode) {
  // 写入目标模式。是否真正切换成功，以 0x6061 反馈为准。
  axis.outData.mode = static_cast<int8_t>(target_mode);

  if (axis.inData.mode_display == static_cast<int8_t>(target_mode)) {
    return FbStatus::kDone;
  }
  return FbStatus::kBusy;
}

}
