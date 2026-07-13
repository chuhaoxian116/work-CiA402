#include "cia402/cia402.h"

int main() {
  cia402::AxisData axis{};
  axis.inData.statusword = 0x0040;
  axis.inData.mode_display = static_cast<int8_t>(cia402::AxisMode::kHoming);

  if (cia402::GetAxisState(axis) != cia402::AxisState::kSwitchOnDisabled) {
    return 1;
  }

  if (cia402::PowerAxis(axis, true) != cia402::FbStatus::kBusy) {
    return 2;
  }
  if (axis.outData.controlword != 0x0006) {
    return 3;
  }

  if (cia402::SwitchMode(axis, cia402::AxisMode::kCyclicSynchronousPosition) !=
      cia402::FbStatus::kBusy) {
    return 4;
  }
  if (axis.outData.mode !=
      static_cast<int8_t>(cia402::AxisMode::kCyclicSynchronousPosition)) {
    return 5;
  }

  return 0;
}
