#include "cia402/cia402.h"

namespace {

bool is_null(const cia402_axis_input_t *input) {
    return input == nullptr;
}

bool is_null(const cia402_axis_output_t *output) {
    return output == nullptr;
}

uint16_t clear_homing_start(uint16_t controlword) {
    return static_cast<uint16_t>(controlword & ~CIA402_CW_HOMING_START);
}

uint16_t set_homing_start(uint16_t controlword) {
    return static_cast<uint16_t>(controlword | CIA402_CW_HOMING_START);
}

}  // namespace

extern "C" {

const char *cia402_version(void) {
    return "0.2.0";
}

cia402_axis_state_t cia402_get_axis_state(
    const cia402_axis_input_t *input) {
    if (is_null(input)) {
        return CIA402_AXIS_STATE_UNKNOWN;
    }
    return cia402_get_axis_state_from_statusword(input->statusword);
}

cia402_axis_state_t cia402_get_axis_state_from_statusword(
    uint16_t statusword) {
    const uint16_t masked = statusword & 0x006F;

    if (masked == 0x0000) {
        return CIA402_AXIS_STATE_NOT_READY_TO_SWITCH_ON;
    }
    if ((statusword & 0x004F) == 0x0040) {
        return CIA402_AXIS_STATE_SWITCH_ON_DISABLED;
    }
    if (masked == 0x0021) {
        return CIA402_AXIS_STATE_READY_TO_SWITCH_ON;
    }
    if (masked == 0x0023) {
        return CIA402_AXIS_STATE_SWITCHED_ON;
    }
    if (masked == 0x0027) {
        return CIA402_AXIS_STATE_OPERATION_ENABLED;
    }
    if (masked == 0x0007) {
        return CIA402_AXIS_STATE_QUICK_STOP_ACTIVE;
    }
    if ((statusword & 0x004F) == 0x000F) {
        return CIA402_AXIS_STATE_FAULT_REACTION_ACTIVE;
    }
    if ((statusword & 0x004F) == 0x0008) {
        return CIA402_AXIS_STATE_FAULT;
    }

    return CIA402_AXIS_STATE_UNKNOWN;
}

cia402_axis_homing_state_t cia402_get_axis_homing_state(
    const cia402_axis_input_t *input) {
    if (is_null(input)) {
        return CIA402_AXIS_HOMING_STATE_UNKNOWN;
    }
    if (input->mode_display != CIA402_AXIS_MODE_HOMING) {
        return CIA402_AXIS_HOMING_STATE_NOT_HOMING_MODE;
    }
    if ((input->statusword & CIA402_SW_HOMING_ERROR) != 0) {
        return CIA402_AXIS_HOMING_STATE_ERROR;
    }
    if ((input->statusword & CIA402_SW_HOMING_ATTAINED) != 0) {
        return CIA402_AXIS_HOMING_STATE_ATTAINED;
    }
    if ((input->statusword & CIA402_SW_TARGET_REACHED) == 0) {
        return CIA402_AXIS_HOMING_STATE_RUNNING;
    }
    return CIA402_AXIS_HOMING_STATE_NOT_STARTED;
}

const char *cia402_axis_state_name(cia402_axis_state_t state) {
    switch (state) {
        case CIA402_AXIS_STATE_NOT_READY_TO_SWITCH_ON:
            return "Not ready to switch on";
        case CIA402_AXIS_STATE_SWITCH_ON_DISABLED:
            return "Switch on disabled";
        case CIA402_AXIS_STATE_READY_TO_SWITCH_ON:
            return "Ready to switch on";
        case CIA402_AXIS_STATE_SWITCHED_ON:
            return "Switched on";
        case CIA402_AXIS_STATE_OPERATION_ENABLED:
            return "Operation enabled";
        case CIA402_AXIS_STATE_QUICK_STOP_ACTIVE:
            return "Quick stop active";
        case CIA402_AXIS_STATE_FAULT_REACTION_ACTIVE:
            return "Fault reaction active";
        case CIA402_AXIS_STATE_FAULT:
            return "Fault";
        case CIA402_AXIS_STATE_UNKNOWN:
        default:
            return "Unknown";
    }
}

const char *cia402_axis_mode_name(cia402_axis_mode_t mode) {
    switch (mode) {
        case CIA402_AXIS_MODE_NONE:
            return "No mode";
        case CIA402_AXIS_MODE_PROFILE_POSITION:
            return "Profile position";
        case CIA402_AXIS_MODE_VELOCITY:
            return "Velocity";
        case CIA402_AXIS_MODE_PROFILE_VELOCITY:
            return "Profile velocity";
        case CIA402_AXIS_MODE_PROFILE_TORQUE:
            return "Profile torque";
        case CIA402_AXIS_MODE_HOMING:
            return "Homing";
        case CIA402_AXIS_MODE_INTERPOLATED_POSITION:
            return "Interpolated position";
        case CIA402_AXIS_MODE_CYCLIC_SYNC_POSITION:
            return "Cyclic synchronous position";
        case CIA402_AXIS_MODE_CYCLIC_SYNC_VELOCITY:
            return "Cyclic synchronous velocity";
        case CIA402_AXIS_MODE_CYCLIC_SYNC_TORQUE:
            return "Cyclic synchronous torque";
        default:
            return "Unknown mode";
    }
}

const char *cia402_axis_homing_state_name(
    cia402_axis_homing_state_t state) {
    switch (state) {
        case CIA402_AXIS_HOMING_STATE_NOT_HOMING_MODE:
            return "Not homing mode";
        case CIA402_AXIS_HOMING_STATE_NOT_STARTED:
            return "Not started";
        case CIA402_AXIS_HOMING_STATE_RUNNING:
            return "Running";
        case CIA402_AXIS_HOMING_STATE_ATTAINED:
            return "Attained";
        case CIA402_AXIS_HOMING_STATE_ERROR:
            return "Error";
        case CIA402_AXIS_HOMING_STATE_UNKNOWN:
        default:
            return "Unknown";
    }
}

const char *cia402_fb_status_name(cia402_fb_status_t status) {
    switch (status) {
        case CIA402_FB_STATUS_DONE:
            return "Done";
        case CIA402_FB_STATUS_BUSY:
            return "Busy";
        case CIA402_FB_STATUS_ERROR:
        default:
            return "Error";
    }
}

int cia402_is_fault(const cia402_axis_input_t *input) {
    return !is_null(input) && ((input->statusword & CIA402_SW_FAULT) != 0);
}

int cia402_is_warning(const cia402_axis_input_t *input) {
    return !is_null(input) && ((input->statusword & CIA402_SW_WARNING) != 0);
}

int cia402_is_remote(const cia402_axis_input_t *input) {
    return !is_null(input) && ((input->statusword & CIA402_SW_REMOTE) != 0);
}

int cia402_is_operation_enabled(const cia402_axis_input_t *input) {
    return cia402_get_axis_state(input) ==
           CIA402_AXIS_STATE_OPERATION_ENABLED;
}

int cia402_is_target_reached(const cia402_axis_input_t *input) {
    return !is_null(input) &&
           ((input->statusword & CIA402_SW_TARGET_REACHED) != 0);
}

int cia402_is_mode_reached(const cia402_axis_input_t *input,
                           cia402_axis_mode_t target_mode) {
    return !is_null(input) && input->mode_display == target_mode;
}

uint16_t cia402_controlword_disable_voltage(void) {
    return 0x0000;
}

uint16_t cia402_controlword_shutdown(void) {
    return 0x0006;
}

uint16_t cia402_controlword_switch_on(void) {
    return 0x0007;
}

uint16_t cia402_controlword_enable_operation(void) {
    return 0x000F;
}

uint16_t cia402_controlword_disable_operation(void) {
    return 0x0007;
}

uint16_t cia402_controlword_quick_stop(void) {
    return 0x0002;
}

uint16_t cia402_controlword_fault_reset(void) {
    return 0x0080;
}

cia402_fb_status_t cia402_power_axis(const cia402_axis_input_t *input,
                                     cia402_axis_output_t *output,
                                     int enable) {
    if (is_null(input) || is_null(output)) {
        return CIA402_FB_STATUS_ERROR;
    }

    const cia402_axis_state_t state = cia402_get_axis_state(input);

    if (!enable) {
        output->controlword = cia402_controlword_disable_operation();
        return state == CIA402_AXIS_STATE_OPERATION_ENABLED
                   ? CIA402_FB_STATUS_BUSY
                   : CIA402_FB_STATUS_DONE;
    }

    if (state == CIA402_AXIS_STATE_FAULT ||
        state == CIA402_AXIS_STATE_FAULT_REACTION_ACTIVE) {
        return CIA402_FB_STATUS_ERROR;
    }

    switch (state) {
        case CIA402_AXIS_STATE_SWITCH_ON_DISABLED:
            output->controlword = cia402_controlword_shutdown();
            return CIA402_FB_STATUS_BUSY;
        case CIA402_AXIS_STATE_READY_TO_SWITCH_ON:
            output->controlword = cia402_controlword_switch_on();
            return CIA402_FB_STATUS_BUSY;
        case CIA402_AXIS_STATE_SWITCHED_ON:
            output->controlword = cia402_controlword_enable_operation();
            return CIA402_FB_STATUS_BUSY;
        case CIA402_AXIS_STATE_OPERATION_ENABLED:
            output->controlword = cia402_controlword_enable_operation();
            return CIA402_FB_STATUS_DONE;
        case CIA402_AXIS_STATE_QUICK_STOP_ACTIVE:
            output->controlword = cia402_controlword_disable_operation();
            return CIA402_FB_STATUS_BUSY;
        case CIA402_AXIS_STATE_NOT_READY_TO_SWITCH_ON:
        case CIA402_AXIS_STATE_UNKNOWN:
        default:
            output->controlword = cia402_controlword_disable_voltage();
            return CIA402_FB_STATUS_BUSY;
    }
}

cia402_fb_status_t cia402_clear_axis_error(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output) {
    if (is_null(input) || is_null(output)) {
        return CIA402_FB_STATUS_ERROR;
    }

    const cia402_axis_state_t state = cia402_get_axis_state(input);
    if (state == CIA402_AXIS_STATE_FAULT) {
        output->controlword = cia402_controlword_fault_reset();
        return CIA402_FB_STATUS_BUSY;
    }
    if (state == CIA402_AXIS_STATE_FAULT_REACTION_ACTIVE) {
        output->controlword = cia402_controlword_disable_voltage();
        return CIA402_FB_STATUS_BUSY;
    }

    output->controlword = cia402_controlword_disable_voltage();
    return CIA402_FB_STATUS_DONE;
}

cia402_fb_status_t cia402_switch_mode(const cia402_axis_input_t *input,
                                      cia402_axis_output_t *output,
                                      cia402_axis_mode_t target_mode) {
    if (is_null(input) || is_null(output)) {
        return CIA402_FB_STATUS_ERROR;
    }

    output->mode = static_cast<int8_t>(target_mode);
    if (input->mode_display == target_mode) {
        return CIA402_FB_STATUS_DONE;
    }
    return CIA402_FB_STATUS_BUSY;
}

cia402_fb_status_t cia402_homing(const cia402_axis_input_t *input,
                                 cia402_axis_output_t *output,
                                 int start) {
    if (is_null(input) || is_null(output)) {
        return CIA402_FB_STATUS_ERROR;
    }

    output->mode = CIA402_AXIS_MODE_HOMING;

    if (!start) {
        output->controlword = clear_homing_start(output->controlword);
        return CIA402_FB_STATUS_DONE;
    }

    const cia402_axis_homing_state_t homing_state =
        cia402_get_axis_homing_state(input);
    if (homing_state == CIA402_AXIS_HOMING_STATE_ERROR) {
        output->controlword = clear_homing_start(output->controlword);
        return CIA402_FB_STATUS_ERROR;
    }
    if (homing_state == CIA402_AXIS_HOMING_STATE_ATTAINED) {
        output->controlword = clear_homing_start(
            cia402_controlword_enable_operation());
        return CIA402_FB_STATUS_DONE;
    }

    if (input->mode_display != CIA402_AXIS_MODE_HOMING) {
        output->controlword = clear_homing_start(output->controlword);
        return CIA402_FB_STATUS_BUSY;
    }

    const cia402_fb_status_t power_status =
        cia402_power_axis(input, output, 1);
    if (power_status == CIA402_FB_STATUS_ERROR) {
        return CIA402_FB_STATUS_ERROR;
    }
    if (power_status == CIA402_FB_STATUS_BUSY) {
        output->controlword = clear_homing_start(output->controlword);
        return CIA402_FB_STATUS_BUSY;
    }

    output->controlword = set_homing_start(output->controlword);
    return CIA402_FB_STATUS_BUSY;
}

}  // extern "C"
