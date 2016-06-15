#define private public
#include "Copter.h"


#define GET_GYRO_X() copter.ins.get_gyro(0).x;
#define GET_GYRO_Y() copter.ins.get_gyro(0).y;
#define GET_GYRO_Z() copter.ins.get_gyro(0).z;

#define GET_ACCEL_X() copter.ins.get_accel(0).x;
#define GET_ACCEL_Y() copter.ins.get_accel(0).y;
#define GET_ACCEL_Z() copter.ins.get_accel(0).z;


float get_pilot_desired_roll();
float get_pilot_desired_pitch();

#define GET_DESIRED_YAW_RATE() copter.get_pilot_desired_yaw_rate(copter.channel_yaw->control_in);
#define GET_DESIRED_THROTLE()  copter.get_pilot_desired_throttle(copter.channel_throttle->control_in);
#define GET_DESIRED_ROLL() get_pilot_desired_roll();
#define GET_DESIRED_PITCH() get_pilot_desired_pitch();

#define SET_YAW_RATE_TARGET(x)   copter.attitude_control.rate_bf_yaw_target(x);
#define SET_ROLL_RATE_TARGET(x)  copter.attitude_control.rate_bf_roll_target(x);
#define SET_PITCH_RATE_TARGET(x) copter.attitude_control.rate_bf_pitch_target(x);

#define GET_ROLL_STATE()  copter.ahrs.roll
#define GET_PITCH_STATE() copter.ahrs.pitch
#define GET_YAW_STATE()   copter.ahrs.yaw