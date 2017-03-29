#include <cvra/motor/config/LoadConfiguration.hpp>
#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <control.h>
#include "main.h"
#include "LoadConfiguration_server.hpp"

static void set_param(const char *name, const float value)
{
    parameter_t *p = parameter_find(&parameter_root_ns, name);
    parameter_scalar_set(p, value);
}

int LoadConfiguration_server_start(Node &node)
{
    int ret;

    static uavcan::ServiceServer<cvra::motor::config::LoadConfiguration> load_config_srv(node);

    ret = load_config_srv.start(
        [&](const uavcan::ReceivedDataStructure<cvra::motor::config::LoadConfiguration::Request>&
            req,
            cvra::motor::config::LoadConfiguration::Response& rsp)
    {
        (void) rsp;

        control_stop();

        set_param("control/acceleration_limit", req.acceleration_limit);
        set_param("control/velocity_limit", req.velocity_limit);
        set_param("control/torque_limit", req.torque_limit);
        set_param("control/low_batt_th", req.low_batt_th);

        set_param("control/current/kp", req.current_pid.kp);
        set_param("control/current/ki", req.current_pid.ki);
        set_param("control/current/kd", req.current_pid.kd);
        set_param("control/current/i_limit", req.current_pid.ilimit);

        set_param("control/velocity/kp", req.velocity_pid.kp);
        set_param("control/velocity/ki", req.velocity_pid.ki);
        set_param("control/velocity/kd", req.velocity_pid.kd);
        set_param("control/velocity/i_limit", req.velocity_pid.ilimit);

        set_param("control/position/kp", req.position_pid.kp);
        set_param("control/position/ki", req.position_pid.ki);
        set_param("control/position/kd", req.position_pid.kd);
        set_param("control/position/i_limit", req.position_pid.ilimit);

        set_param("motor/torque_cst", req.torque_constant);

        set_param("thermal/current_gain", req.thermal_current_gain);
        set_param("thermal/max_temp", req.max_temperature);
        set_param("thermal/Rth", req.thermal_resistance);
        set_param("thermal/Cth",
                  req.thermal_capacity);

        if (req.mode == cvra::motor::config::LoadConfiguration::Request::MODE_INDEX) {
            control_feedback.input_selection = FEEDBACK_RPM;
        } else if (req.mode == cvra::motor::config::LoadConfiguration::Request::MODE_ENC_PERIODIC) {
            control_feedback.input_selection = FEEDBACK_PRIMARY_ENCODER_PERIODIC;
        } else if (req.mode == cvra::motor::config::LoadConfiguration::Request::MODE_ENC_BOUNDED) {
            control_feedback.input_selection = FEEDBACK_PRIMARY_ENCODER_BOUNDED;
        } else if (req.mode == cvra::motor::config::LoadConfiguration::Request::MODE_2_ENC_PERIODIC) {
            control_feedback.input_selection = FEEDBACK_TWO_ENCODERS_PERIODIC;
        } else if (req.mode == cvra::motor::config::LoadConfiguration::Request::MODE_MOTOR_POT) {
            control_feedback.input_selection = FEEDBACK_POTENTIOMETER;
        }


        control_feedback.primary_encoder.transmission_p = req.transmission_ratio_p;
        control_feedback.primary_encoder.transmission_q = req.transmission_ratio_q;
        control_feedback.primary_encoder.ticks_per_rev = req.motor_encoder_steps_per_revolution;

        control_feedback.secondary_encoder.transmission_p = 1;
        control_feedback.secondary_encoder.transmission_q = 1;
        control_feedback.secondary_encoder.ticks_per_rev = req.second_encoder_steps_per_revolution;

        control_feedback.potentiometer.gain = req.potentiometer_gain;
        control_feedback.potentiometer.zero = 0;

        control_feedback.rpm.phase = 0;

        control_start();

        chprintf(ch_stdout, "LoadConfiguration received\n");

        // Mark the node as correctly initialized
        node.getNodeStatusProvider().setModeOperational();
        node.getNodeStatusProvider().setHealthOk();
    });

    return ret;
}