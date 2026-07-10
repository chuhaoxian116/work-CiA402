#ifndef CIA402_CIA402_HPP_
#define CIA402_CIA402_HPP_

#include "cia402/cia402.h"

namespace cia402 {

using AxisInput = cia402_axis_input_t;
using AxisOutput = cia402_axis_output_t;

enum class AxisMode {
    None = CIA402_AXIS_MODE_NONE,
    ProfilePosition = CIA402_AXIS_MODE_PROFILE_POSITION,
    Velocity = CIA402_AXIS_MODE_VELOCITY,
    ProfileVelocity = CIA402_AXIS_MODE_PROFILE_VELOCITY,
    ProfileTorque = CIA402_AXIS_MODE_PROFILE_TORQUE,
    Homing = CIA402_AXIS_MODE_HOMING,
    InterpolatedPosition = CIA402_AXIS_MODE_INTERPOLATED_POSITION,
    CyclicSyncPosition = CIA402_AXIS_MODE_CYCLIC_SYNC_POSITION,
    CyclicSyncVelocity = CIA402_AXIS_MODE_CYCLIC_SYNC_VELOCITY,
    CyclicSyncTorque = CIA402_AXIS_MODE_CYCLIC_SYNC_TORQUE,
};

enum class AxisState {
    NotReadyToSwitchOn = CIA402_AXIS_STATE_NOT_READY_TO_SWITCH_ON,
    SwitchOnDisabled = CIA402_AXIS_STATE_SWITCH_ON_DISABLED,
    ReadyToSwitchOn = CIA402_AXIS_STATE_READY_TO_SWITCH_ON,
    SwitchedOn = CIA402_AXIS_STATE_SWITCHED_ON,
    OperationEnabled = CIA402_AXIS_STATE_OPERATION_ENABLED,
    QuickStopActive = CIA402_AXIS_STATE_QUICK_STOP_ACTIVE,
    FaultReactionActive = CIA402_AXIS_STATE_FAULT_REACTION_ACTIVE,
    Fault = CIA402_AXIS_STATE_FAULT,
    Unknown = CIA402_AXIS_STATE_UNKNOWN,
};

enum class AxisHomingState {
    NotHomingMode = CIA402_AXIS_HOMING_STATE_NOT_HOMING_MODE,
    NotStarted = CIA402_AXIS_HOMING_STATE_NOT_STARTED,
    Running = CIA402_AXIS_HOMING_STATE_RUNNING,
    Attained = CIA402_AXIS_HOMING_STATE_ATTAINED,
    Error = CIA402_AXIS_HOMING_STATE_ERROR,
    Unknown = CIA402_AXIS_HOMING_STATE_UNKNOWN,
};

enum class FbStatus {
    Done = CIA402_FB_STATUS_DONE,
    Busy = CIA402_FB_STATUS_BUSY,
    Error = CIA402_FB_STATUS_ERROR,
};

inline AxisState get_axis_state(const AxisInput &input) {
    return static_cast<AxisState>(cia402_get_axis_state(&input));
}

inline AxisHomingState get_axis_homing_state(const AxisInput &input) {
    return static_cast<AxisHomingState>(cia402_get_axis_homing_state(&input));
}

inline const char *axis_state_name(AxisState state) {
    return cia402_axis_state_name(static_cast<cia402_axis_state_t>(state));
}

inline const char *axis_mode_name(AxisMode mode) {
    return cia402_axis_mode_name(static_cast<cia402_axis_mode_t>(mode));
}

inline const char *axis_homing_state_name(AxisHomingState state) {
    return cia402_axis_homing_state_name(
        static_cast<cia402_axis_homing_state_t>(state));
}

inline const char *fb_status_name(FbStatus status) {
    return cia402_fb_status_name(static_cast<cia402_fb_status_t>(status));
}

inline FbStatus power_axis(const AxisInput &input, AxisOutput &output,
                           bool enable) {
    return static_cast<FbStatus>(
        cia402_power_axis(&input, &output, enable ? 1 : 0));
}

inline FbStatus clear_axis_error(const AxisInput &input, AxisOutput &output) {
    return static_cast<FbStatus>(cia402_clear_axis_error(&input, &output));
}

inline FbStatus switch_mode(const AxisInput &input, AxisOutput &output,
                            AxisMode target_mode) {
    return static_cast<FbStatus>(cia402_switch_mode(
        &input, &output, static_cast<cia402_axis_mode_t>(target_mode)));
}

inline FbStatus homing(const AxisInput &input, AxisOutput &output, bool start) {
    return static_cast<FbStatus>(
        cia402_homing(&input, &output, start ? 1 : 0));
}

class AxisStatus {
public:
    explicit AxisStatus(const AxisInput &input) : input_(input) {}

    uint16_t statusword() const { return input_.statusword; }
    int8_t mode_display() const { return input_.mode_display; }
    AxisState state() const { return get_axis_state(input_); }
    AxisHomingState homing_state() const { return get_axis_homing_state(input_); }

    bool fault() const { return cia402_is_fault(&input_) != 0; }
    bool warning() const { return cia402_is_warning(&input_) != 0; }
    bool remote() const { return cia402_is_remote(&input_) != 0; }
    bool operation_enabled() const {
        return cia402_is_operation_enabled(&input_) != 0;
    }
    bool target_reached() const {
        return cia402_is_target_reached(&input_) != 0;
    }
    bool mode_reached(AxisMode mode) const {
        return cia402_is_mode_reached(
                   &input_, static_cast<cia402_axis_mode_t>(mode)) != 0;
    }

private:
    AxisInput input_;
};

}  // namespace cia402

#endif  // CIA402_CIA402_HPP_
