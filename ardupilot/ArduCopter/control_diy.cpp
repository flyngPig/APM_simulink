#include modelname.h

bool Copter::diy_init(bool ignore_checks)
{
	MDL_initialization();
	return true;
}

void Copter::diy_run()
{
	MDL_update();
	MDL_output();
}	