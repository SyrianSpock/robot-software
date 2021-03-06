#include <cvra/motor/config/PositionPID.hpp>
#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include <control.h>
#include "main.h"
#include "PositionPID_server.hpp"

static void set_param(const char *name, const float value)
{
    parameter_t *p = parameter_find(&parameter_root_ns, name);
    parameter_scalar_set(p, value);
}

int PositionPID_server_start(Node &node)
{
    int ret;

    static uavcan::ServiceServer<cvra::motor::config::PositionPID> load_config_srv(node);

    ret = load_config_srv.start(
        [&](const uavcan::ReceivedDataStructure<cvra::motor::config::PositionPID::Request>&
            req,
            cvra::motor::config::PositionPID::Response& rsp)
    {

        (void) rsp;     /* empty response */
        set_param("/control/position/kp", req.pid.kp);
        set_param("/control/position/ki", req.pid.ki);
        set_param("/control/position/kd", req.pid.kd);
        set_param("/control/position/i_limit", req.pid.ilimit);
    });

    return ret;
}
