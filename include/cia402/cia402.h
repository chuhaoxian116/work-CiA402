#ifndef CIA402_CIA402_H_
#define CIA402_CIA402_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  if defined(CIA402_BUILDING_LIBRARY)
#    define CIA402_API __declspec(dllexport)
#  else
#    define CIA402_API __declspec(dllimport)
#  endif
#else
#  define CIA402_API __attribute__((visibility("default")))
#endif

/**
 * @brief CiA 402 standard operation modes.
 *
 * These values are normally written to 0x6060 "Modes of operation" and read
 * back from 0x6061 "Modes of operation display".
 */
typedef enum cia402_axis_mode {
    CIA402_AXIS_MODE_NONE = 0,
    CIA402_AXIS_MODE_PROFILE_POSITION = 1,
    CIA402_AXIS_MODE_VELOCITY = 2,
    CIA402_AXIS_MODE_PROFILE_VELOCITY = 3,
    CIA402_AXIS_MODE_PROFILE_TORQUE = 4,
    CIA402_AXIS_MODE_HOMING = 6,
    CIA402_AXIS_MODE_INTERPOLATED_POSITION = 7,
    CIA402_AXIS_MODE_CYCLIC_SYNC_POSITION = 8,
    CIA402_AXIS_MODE_CYCLIC_SYNC_VELOCITY = 9,
    CIA402_AXIS_MODE_CYCLIC_SYNC_TORQUE = 10
} cia402_axis_mode_t;

/**
 * @brief CiA 402 power state decoded from 0x6041 statusword.
 */
typedef enum cia402_axis_state {
    CIA402_AXIS_STATE_NOT_READY_TO_SWITCH_ON = 0,
    CIA402_AXIS_STATE_SWITCH_ON_DISABLED,
    CIA402_AXIS_STATE_READY_TO_SWITCH_ON,
    CIA402_AXIS_STATE_SWITCHED_ON,
    CIA402_AXIS_STATE_OPERATION_ENABLED,
    CIA402_AXIS_STATE_QUICK_STOP_ACTIVE,
    CIA402_AXIS_STATE_FAULT_REACTION_ACTIVE,
    CIA402_AXIS_STATE_FAULT,
    CIA402_AXIS_STATE_UNKNOWN
} cia402_axis_state_t;

/**
 * @brief Homing state decoded from mode display and 0x6041 statusword bits.
 */
typedef enum cia402_axis_homing_state {
    CIA402_AXIS_HOMING_STATE_NOT_HOMING_MODE = 0,
    CIA402_AXIS_HOMING_STATE_NOT_STARTED,
    CIA402_AXIS_HOMING_STATE_RUNNING,
    CIA402_AXIS_HOMING_STATE_ATTAINED,
    CIA402_AXIS_HOMING_STATE_ERROR,
    CIA402_AXIS_HOMING_STATE_UNKNOWN
} cia402_axis_homing_state_t;

/**
 * @brief One-step function block result.
 *
 * The function blocks in this library are intentionally non-blocking. Call them
 * once per control cycle. They update AxisOutput and report whether the request
 * is done, still busy, or impossible because the drive reports a fault/error.
 */
typedef enum cia402_fb_status {
    CIA402_FB_STATUS_DONE = 0,
    CIA402_FB_STATUS_BUSY,
    CIA402_FB_STATUS_ERROR
} cia402_fb_status_t;

/**
 * @brief Minimal CiA 402 input PDO values used by this library.
 */
typedef struct cia402_axis_input {
    uint16_t statusword;      /* 0x6041 */
    int8_t mode_display;      /* 0x6061 */
} cia402_axis_input_t;

/**
 * @brief Minimal CiA 402 output PDO values written by this library.
 */
typedef struct cia402_axis_output {
    uint16_t controlword;     /* 0x6040 */
    int8_t mode;              /* 0x6060 */
} cia402_axis_output_t;

/**
 * @brief Standard controlword bit masks for object 0x6040.
 */
enum cia402_controlword_bits {
    CIA402_CW_SWITCH_ON = 0x0001,
    CIA402_CW_ENABLE_VOLTAGE = 0x0002,
    CIA402_CW_QUICK_STOP = 0x0004,
    CIA402_CW_ENABLE_OPERATION = 0x0008,
    CIA402_CW_HOMING_START = 0x0010,
    CIA402_CW_FAULT_RESET = 0x0080,
    CIA402_CW_HALT = 0x0100
};

/**
 * @brief Standard statusword bit masks for object 0x6041.
 */
enum cia402_statusword_bits {
    CIA402_SW_READY_TO_SWITCH_ON = 0x0001,
    CIA402_SW_SWITCHED_ON = 0x0002,
    CIA402_SW_OPERATION_ENABLED = 0x0004,
    CIA402_SW_FAULT = 0x0008,
    CIA402_SW_VOLTAGE_ENABLED = 0x0010,
    CIA402_SW_QUICK_STOP = 0x0020,
    CIA402_SW_SWITCH_ON_DISABLED = 0x0040,
    CIA402_SW_WARNING = 0x0080,
    CIA402_SW_REMOTE = 0x0200,
    CIA402_SW_TARGET_REACHED = 0x0400,
    CIA402_SW_INTERNAL_LIMIT_ACTIVE = 0x0800,
    CIA402_SW_HOMING_ATTAINED = 0x1000,
    CIA402_SW_HOMING_ERROR = 0x2000
};

CIA402_API const char *cia402_version(void);

CIA402_API cia402_axis_state_t cia402_get_axis_state(
    const cia402_axis_input_t *input);
CIA402_API cia402_axis_state_t cia402_get_axis_state_from_statusword(
    uint16_t statusword);
CIA402_API cia402_axis_homing_state_t cia402_get_axis_homing_state(
    const cia402_axis_input_t *input);

CIA402_API const char *cia402_axis_state_name(cia402_axis_state_t state);
CIA402_API const char *cia402_axis_mode_name(cia402_axis_mode_t mode);
CIA402_API const char *cia402_axis_homing_state_name(
    cia402_axis_homing_state_t state);
CIA402_API const char *cia402_fb_status_name(cia402_fb_status_t status);

CIA402_API int cia402_is_fault(const cia402_axis_input_t *input);
CIA402_API int cia402_is_warning(const cia402_axis_input_t *input);
CIA402_API int cia402_is_remote(const cia402_axis_input_t *input);
CIA402_API int cia402_is_operation_enabled(const cia402_axis_input_t *input);
CIA402_API int cia402_is_target_reached(const cia402_axis_input_t *input);
CIA402_API int cia402_is_mode_reached(const cia402_axis_input_t *input,
                                      cia402_axis_mode_t target_mode);

CIA402_API uint16_t cia402_controlword_disable_voltage(void);
CIA402_API uint16_t cia402_controlword_shutdown(void);
CIA402_API uint16_t cia402_controlword_switch_on(void);
CIA402_API uint16_t cia402_controlword_enable_operation(void);
CIA402_API uint16_t cia402_controlword_disable_operation(void);
CIA402_API uint16_t cia402_controlword_quick_stop(void);
CIA402_API uint16_t cia402_controlword_fault_reset(void);

/**
 * @brief One-cycle power function block.
 *
 * enable = 1:
 *   Switch on disabled -> shutdown(0x0006)
 *   Ready to switch on -> switch on(0x0007)
 *   Switched on        -> enable operation(0x000F)
 *   Operation enabled  -> keep enable operation(0x000F), DONE
 *
 * enable = 0:
 *   output disable operation(0x0007), DONE when not operation enabled.
 */
CIA402_API cia402_fb_status_t cia402_power_axis(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output,
    int enable);

/**
 * @brief One-cycle fault reset function block.
 *
 * If the drive is in Fault, writes 0x0080 and returns BUSY. When the fault bit
 * disappears, writes 0 and returns DONE.
 */
CIA402_API cia402_fb_status_t cia402_clear_axis_error(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output);

/**
 * @brief One-cycle mode switch function block.
 *
 * Writes target mode to 0x6060. Returns DONE when 0x6061 equals target mode.
 * This function does not decide whether the axis should be powered before or
 * after mode switching; that policy remains in the caller.
 */
CIA402_API cia402_fb_status_t cia402_switch_mode(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output,
    cia402_axis_mode_t target_mode);

/**
 * @brief One-cycle homing function block.
 *
 * Assumptions:
 * - Homing parameters such as 0x6098/0x6099/0x609A are configured elsewhere.
 * - This library only controls 0x6060 and 0x6040.
 *
 * start = 1:
 *   switch to Homing mode, power axis, then set controlword bit4.
 *   DONE when standard homing attained bit is set.
 *
 * start = 0:
 *   clear homing start bit.
 */
CIA402_API cia402_fb_status_t cia402_homing(
    const cia402_axis_input_t *input,
    cia402_axis_output_t *output,
    int start);

#ifdef __cplusplus
}
#endif

#endif  // CIA402_CIA402_H_
