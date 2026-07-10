#ifndef CIA402_CIA402_H_
#define CIA402_CIA402_H_

#include <cstdint>

namespace cia402 {

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

enum class AxisHomingState {
  kNotHomingMode,
  kNotStarted,
  kRunning,
  kAttained,
  kError,
  kUnknown,
};

enum class FbStatus {
  kDone,
  kBusy,
  kError,
};

struct AxisInput {
  uint16_t statusword = 0;  // 0x6041 Statusword
  int8_t mode_display = static_cast<int8_t>(
      AxisMode::kNone);  // 0x6061 Modes of operation display
};

struct AxisOutput {
  uint16_t controlword = 0;  // 0x6040 Controlword
  int8_t mode =
      static_cast<int8_t>(AxisMode::kNone);  // 0x6060 Modes of operation
};

const char* Version();

AxisState GetAxisState(const AxisInput& input);
AxisState GetAxisStateFromStatusword(uint16_t statusword);
AxisHomingState GetAxisHomingState(const AxisInput& input);

const char* ToString(AxisMode mode);
const char* ToString(AxisState state);
const char* ToString(AxisHomingState state);
const char* ToString(FbStatus status);

bool IsFault(const AxisInput& input);
bool IsWarning(const AxisInput& input);
bool IsRemote(const AxisInput& input);
bool IsOperationEnabled(const AxisInput& input);
bool IsTargetReached(const AxisInput& input);
bool IsModeReached(const AxisInput& input, AxisMode target_mode);

class PowerAxis {
 public:
  FbStatus Update(const AxisInput& input, AxisOutput& output, bool enable);
};

class ClearAxisError {
 public:
  FbStatus Update(const AxisInput& input, AxisOutput& output);
};

class SwitchMode {
 public:
  FbStatus Update(const AxisInput& input, AxisOutput& output,
                  AxisMode target_mode);
};

class Homing {
 public:
  FbStatus Update(const AxisInput& input, AxisOutput& output, bool start);
};

}  // namespace cia402

#endif  // CIA402_CIA402_H_
