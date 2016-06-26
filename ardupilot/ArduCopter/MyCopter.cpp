#include "MyCopter.h"

int c = 0;

float target_roll;
float target_pitch;

float get_pilot_desired_roll(){
	copter.get_pilot_desired_lean_angles(copter.channel_roll->control_in, copter.channel_pitch->control_in, target_roll, target_pitch, copter.aparm.angle_max);

	return target_roll;
}

float get_pilot_desired_pitch(){
	copter.get_pilot_desired_lean_angles(copter.channel_roll->control_in, copter.channel_pitch->control_in, target_roll, target_pitch, copter.aparm.angle_max);

	return target_pitch;
}
