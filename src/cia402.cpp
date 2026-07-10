#include "cia402/cia402.h"

namespace cia402 {
namespace {

constexpr uint16_t kControlwordHomingStartBit = 0x0010;

constexpr uint16_t kStatuswordFaultBit = 0x0008;
constexpr uint16_t kStatuswordWarningBit = 0x0080;
constexpr uint16_t kStatuswordRemoteBit = 0x0200;
constexpr uint16_t kStatuswordTargetReachedBit = 0x0400;
constexpr uint16_t kStatuswordHomingAttainedBit = 0x1000;
constexpr uint16_t kStatuswordHomingErrorBit = 0x2000;

constexpr uint16_t kControlwordDisableVoltage = 0x0000;
constexpr uint16_t kControlwordShutdown = 0x0006;
constexpr uint16_t kControlwordSwitchOn = 0x0007;
constexpr uint16_t kControlwordEnableOperation = 0x000F;
constexpr uint16_t kControlwordDisableOperation = 0x0007;
constexpr uint16_t kControlwordFaultReset = 0x0080;

uint16_t ClearHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword & ~kControlwordHomingStartBit);
}

uint16_t SetHomingStart(uint16_t controlword) {
  return static_cast<uint16_t>(controlword | kControlwordHomingStartBit);
}

}  // namespace

const char* Version() { return "0.3.0"; }

AxisState GetAxisState(const AxisInput& input) {
  return GetAxisStateFromStatusword(input.statusword);
}

AxisState GetAxisStateFromStatusword(uint16_t statusword) {
  const uint16_t masked = statusword & 0x006F;

  if (masked == 0x0000) {
    return AxisState::kNotReadyToSwitchOn;
  }
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

  return AxisState::kUnknown;
}

AxisHomingState GetAxisHomingState(const AxisInput& input) {
  if (input.mode_display != static_cast<int8_t>(AxisMode::kHoming)) {
    return AxisHomingState::kNotHomingMode;
  }
  if ((input.statusword & kStatuswordHomingErrorBit) != 0) {
    return AxisHomingState::kError;
  }
  if ((input.statusword & kStatuswordHomingAttainedBit) != 0) {
    return AxisHomingState::kAttained;
  }
  if ((input.statusword & kStatuswordTargetReachedBit) == 0) {
    return AxisHomingState::kRunning;
  }
  return AxisHomingState::kNotStarted;
}

const char* ToString(AxisMode mode) {
  switch (mode) {
    case AxisMode::kNone:
      return "No mode";
    case AxisMode::kProfilePosition:
      return "Profile position";
    case AxisMode::kVelocity:
      return "Velocity";
    case AxisMode::kProfileVelocity:
      return "Profile velocity";
    case AxisMode::kProfileTorque:
      return "Profile torque";
    case AxisMode::kHoming:
      return "Homing";
    case AxisMode::kInterpolatedPosition:
      return "Interpolated position";
    case AxisMode::kCyclicSyncPosition:
      return "Cyclic synchronous position";
    case AxisMode::kCyclicSyncVelocity:
      return "Cyclic synchronous velocity";
    case AxisMode::kCyclicSyncTorque:
      return "Cyclic synchronous torque";
  }

  return "Unknown mode";
}

const char* ToString(AxisState state) {
  switch (state) {
    case AxisState::kNotReadyToSwitchOn:
      return "Not ready to switch on";
    case AxisState::kSwitchOnDisabled:
      return "Switch on disabled";
    case AxisState::kReadyToSwitchOn:
      return "Ready to switch on";
    case AxisState::kSwitchedOn:
      return "Switched on";
    case AxisState::kOperationEnabled:
      return "Operation enabled";
    case AxisState::kQuickStopActive:
      return "Quick stop active";
    case AxisState::kFaultReactionActive:
      return "Fault reaction active";
    case AxisState::kFault:
      return "Fault";
    case AxisState::kUnknown:
      return "Unknown";
  }

  return "Unknown";
}

const char* ToString(AxisHomingState state) {
  switch (state) {
    case AxisHomingState::kNotHomingMode:
      return "Not homing mode";
    case AxisHomingState::kNotStarted:
      return "Not started";
    case AxisHomingState::kRunning:
      return "Running";
    case AxisHomingState::kAttained:
      return "Attained";
    case AxisHomingState::kError:
      return "Error";
    case AxisHomingState::kUnknown:
      return "Unknown";
  }

  return "Unknown";
}

const char* ToString(FbStatus status) {
  switch (status) {
    case FbStatus::kDone:
      return "Done";
    case FbStatus::kBusy:
      return "Busy";
    case FbStatus::kError:
      return "Error";
  }

  return "Error";
}

bool IsFault(const AxisInput& input) {
  return (input.statusword & kStatuswordFaultBit) != 0;
}

bool IsWarning(const AxisInput& input) {
  return (input.statusword & kStatuswordWarningBit) != 0;
}

bool IsRemote(const AxisInput& input) {
  return (input.statusword & kStatuswordRemoteBit) != 0;
}

bool IsOperationEnabled(const AxisInput& input) {
  return GetAxisState(input) == AxisState::kOperationEnabled;
}

bool IsTargetReached(const AxisInput& input) {
  return (input.statusword & kStatuswordTargetReachedBit) != 0;
}

bool IsModeReached(const AxisInput& input, AxisMode target_mode) {
  return input.mode_display == static_cast<int8_t>(target_mode);
}

FbStatus PowerAxis::Update(const AxisInput& input, AxisOutput& output,
                           bool enable) {
  const AxisState state = GetAxisState(input);

  if (!enable) {
    output.controlword = kControlwordDisableOperation;
    return state == AxisState::kOperationEnabled ? FbStatus::kBusy
                                                 : FbStatus::kDone;
  }

  if (state == AxisState::kFault || state == AxisState::kFaultReactionActive) {
    return FbStatus::kError;
  }

  switch (state) {
    case AxisState::kSwitchOnDisabled:
      output.controlword = kControlwordShutdown;
      return FbStatus::kBusy;
    case AxisState::kReadyToSwitchOn:
      output.controlword = kControlwordSwitchOn;
      return FbStatus::kBusy;
    case AxisState::kSwitchedOn:
      output.controlword = kControlwordEnableOperation;
      return FbStatus::kBusy;
    case AxisState::kOperationEnabled:
      output.controlword = kControlwordEnableOperation;
      return FbStatus::kDone;
    case AxisState::kQuickStopActive:
      output.controlword = kControlwordDisableOperation;
      return FbStatus::kBusy;
    case AxisState::kNotReadyToSwitchOn:
    case AxisState::kUnknown:
    case AxisState::kFaultReactionActive:
    case AxisState::kFault:
      output.controlword = kControlwordDisableVoltage;
      return FbStatus::kBusy;
  }

  output.controlword = kControlwordDisableVoltage;
  return FbStatus::kBusy;
}

FbStatus ClearAxisError::Update(const AxisInput& input, AxisOutput& output) {
  const AxisState state = GetAxisState(input);

  if (state == AxisState::kFault) {
    output.controlword = kControlwordFaultReset;
    return FbStatus::kBusy;
  }
  if (state == AxisState::kFaultReactionActive) {
    output.controlword = kControlwordDisableVoltage;
    return FbStatus::kBusy;
  }

  output.controlword = kControlwordDisableVoltage;
  return FbStatus::kDone;
}

FbStatus SwitchMode::Update(const AxisInput& input, AxisOutput& output,
                            AxisMode target_mode) {
  output.mode = static_cast<int8_t>(target_mode);

  if (IsModeReached(input, target_mode)) {
    return FbStatus::kDone;
  }
  return FbStatus::kBusy;
}

FbStatus Homing::Update(const AxisInput& input, AxisOutput& output,
                        bool start) {
  output.mode = static_cast<int8_t>(AxisMode::kHoming);

  if (!start) {
    output.controlword = ClearHomingStart(output.controlword);
    return FbStatus::kDone;
  }

  const AxisHomingState homing_state = GetAxisHomingState(input);
  if (homing_state == AxisHomingState::kError) {
    output.controlword = ClearHomingStart(output.controlword);
    return FbStatus::kError;
  }
  if (homing_state == AxisHomingState::kAttained) {
    output.controlword = ClearHomingStart(kControlwordEnableOperation);
    return FbStatus::kDone;
  }

  if (!IsModeReached(input, AxisMode::kHoming)) {
    output.controlword = ClearHomingStart(output.controlword);
    return FbStatus::kBusy;
  }

  PowerAxis power_axis;
  const FbStatus power_status = power_axis.Update(input, output, true);
  if (power_status == FbStatus::kError) {
    return FbStatus::kError;
  }
  if (power_status == FbStatus::kBusy) {
    output.controlword = ClearHomingStart(output.controlword);
    return FbStatus::kBusy;
  }

  output.controlword = SetHomingStart(output.controlword);
  return FbStatus::kBusy;
}

}  // namespace cia402
