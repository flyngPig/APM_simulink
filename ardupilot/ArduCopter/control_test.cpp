/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-

#include "Copter.h"
#include <AP_HAL/RCOutput.h>

//copy of stabilize mode
//Amit 26/12/15
bool Copter::test_init(bool ignore_checks)
{
	if (position_ok() || ignore_checks) {
			// TODO: init actions
	        return true;
	    }else{
	    	// the motors aren't armed so the engins won't work.
	    	//though the run function will get into action.
	        return false;
	    }

}

void Copter::test_run()
{
 /* 1. convert user input found in (g.rc_1.control_in,.., g.rc_8.control_in) into
	 lean angle, rotation rate, climb rate, etc that is appropriate for this flight mode

	2. pass these desired angles, rates etc into Attitude Control or Position Control
		on position control mostly z-axis methods will be called because 3D position flight modes make use of the AC_WPNav library.

		If any methods in AC_PosControl are called then the flight mode code must also call:
		update_z_controller()
		and if any xy-axis methods are called then we must call:
		update_xy_controller()
	3. call rate_controller_run() (in AC_AttitudeControl)this converts the output from
	 the methods listed above into roll, pitch and yaw inputs which are sent to the AP_Motors library*/

}
