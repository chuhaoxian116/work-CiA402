#include "cia402/cia402.h"

#include <cstdint>
#include <iostream>

int main() {
  cia402::AxisInput input{};
  input.statusword = 0x0040;  // Switch on disabled
  input.mode_display = static_cast<int8_t>(cia402::AxisMode::kNone);

  cia402::AxisOutput output{};

  std::cout << "state: "
            << cia402::ToString(cia402::GetAxisState(input)) << "\n";

  cia402::PowerAxis power_axis;
  const cia402::FbStatus power_status =
      power_axis.Update(input, output, true);
  std::cout << "power status: " << cia402::ToString(power_status)
            << ", controlword: 0x" << std::hex << output.controlword << "\n";

  cia402::SwitchMode switch_mode;
  const cia402::FbStatus mode_status =
      switch_mode.Update(input, output,
                         cia402::AxisMode::kCyclicSyncPosition);
  std::cout << "switch mode status: " << cia402::ToString(mode_status)
            << ", mode: " << std::dec << static_cast<int>(output.mode)
            << "\n";

  return 0;
}
