#include "DevAxis.h"
#include "libslic3r/LocalesUtils.hpp"

#include "slic3r/GUI/DeviceManager.hpp"
#include "slic3r/Utils/NetworkAgent.hpp"

namespace Slic3r
{

int DevAxis::Ctrl_GoHome()
{
    if (m_is_support_mqtt_homing) {
        json j;
        j["print"]["command"] = "back_to_center";
        j["print"]["sequence_id"] = std::to_string(MachineObject::m_sequence_id++);
        return m_owner->publish_json(j);
    }

    // gcode command
    return m_owner->is_in_printing() ? m_owner->publish_gcode("G28 X\n") : m_owner->publish_gcode("G28 \n");
}

int DevAxis::Ctrl_Axis(std::string axis, double unit, double input_val, int speed)
{
    if (m_is_support_mqtt_axis_ctrl) {
        int dir = input_val > 0 ? 1 : -1;
        if (!IsArchCoreXY()) {
            if (axis.compare("Y") == 0 || axis.compare("Z") == 0) {
                dir = -dir;
            }
        }
        json j;
        j["print"]["command"] = "xyz_ctrl";
        j["print"]["axis"] = axis;
        j["print"]["dir"] = dir;
        j["print"]["mode"] = (std::abs(input_val) >= 10) ? 1 : 0;
        j["print"]["sequence_id"] = std::to_string(MachineObject::m_sequence_id++);
        return m_owner->publish_json(j);
    }

    double value = input_val;
    if (!IsArchCoreXY()) {
        if (axis.compare("Y") == 0 || axis.compare("Z") == 0) {
            value = -1.0 * input_val;
        }
    }

    // G-code needs a '.' decimal point; sprintf("%0.1f") follows the UI locale (e.g. "10,0" in pt_BR).
    const std::string distance = float_to_string_decimal_point(value * unit, 1);
    std::string       cmd;
    if (axis.compare("X") == 0 || axis.compare("Y") == 0 || axis.compare("Z") == 0) {
        cmd = "M211 S \nM211 X1 Y1 Z1\nM1002 push_ref_mode\nG91 \nG1 " + axis + distance + " F" + std::to_string(speed) + "\nM1002 pop_ref_mode\nM211 R\n";
    } else if (axis.compare("E") == 0) {
        cmd = "M83 \nG0 " + axis + distance + " F" + std::to_string(speed) + "\n";
    } else {
        return -1;
    }

    // Orca: strip analytics telemetry (matches MachineObject::command_axis_control)
    return m_owner->publish_gcode(cmd);
}

} // namespace Slic3r