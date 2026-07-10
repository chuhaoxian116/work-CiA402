#include "cia402/cia402.hpp"

#include <cstdint>
#include <iostream>

int main() {
    cia402::AxisInput input{};
    input.statusword = 0x0040;  // Switch on disabled
    input.mode_display = CIA402_AXIS_MODE_NONE;

    cia402::AxisOutput output{};
    output.controlword = 0;
    output.mode = CIA402_AXIS_MODE_NONE;

    const auto state = cia402::AxisStatus(input);
    std::cout << "state: "
              << cia402::axis_state_name(state.state()) << "\n";

    const auto power_status = cia402::power_axis(input, output, true);
    std::cout << "power status: "
              << cia402::fb_status_name(power_status)
              << ", controlword: 0x" << std::hex
              << output.controlword << "\n";

    const auto mode_status =
        cia402::switch_mode(input, output,
                            cia402::AxisMode::CyclicSyncPosition);
    std::cout << "switch mode status: "
              << cia402::fb_status_name(mode_status)
              << ", mode: " << std::dec
              << static_cast<int>(output.mode) << "\n";

    return 0;
}
