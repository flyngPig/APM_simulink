/*
  AP_PerfMon
  Code by Randy Mackay
*/

#include <AP_Common/AP_Common.h>
#include <AP_Param/AP_Param.h>
#include <AP_Math/AP_Math.h>
#include <AP_HAL/AP_HAL.h>
#include <StorageManager/StorageManager.h>
#include <AP_PerfMon/AP_PerfMon.h>        // PerfMonitor library

const AP_HAL::HAL& hal = AP_HAL::get_HAL();

AP_PERFMON_REGISTER_FN(setup)
AP_PERFMON_REGISTER_FN(loop)
AP_PERFMON_REGISTER_FN(testFn)
AP_PERFMON_REGISTER_FN(testFn2)

void setup()
{
  AP_PERFMON_FUNCTION(setup)

  hal.console->print("Performance Monitor test v1.1\n");
}

void loop()
{
    AP_PERFMON_FUNCTION(loop)

    int16_t i = 0;

    for( i=0; i<10; i++ ) {
        testFn();
    }

    AP_PerfMon::DisplayAndClear(5);
    //AP_PerfMon::DisplayResults();
    //AP_PerfMon::ClearAll();

    hal.scheduler->delay(2000);
}

void testFn()
{
    AP_PERFMON_FUNCTION(testFn)
    hal.scheduler->delay(10);
    testFn2();
    hal.scheduler->delay(10);
}

void testFn2()
{
    AP_PERFMON_FUNCTION(testFn2)
    hal.scheduler->delay(10);
}

AP_HAL_MAIN();
