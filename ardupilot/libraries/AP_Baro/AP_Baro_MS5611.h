/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
#pragma once

#include "AP_Baro_Backend.h"

#include <AP_HAL/AP_HAL.h>

/** Abstract serial bus device driver for I2C/SPI. */
class AP_SerialBus
{
public:
    /** Initialize the driver. */
    virtual void init() = 0;

    /** Read a 16-bit value from register "reg". */
    virtual uint16_t read_16bits(uint8_t reg) = 0;

    /** Read a 24-bit value */
    virtual uint32_t read_24bits(uint8_t reg) = 0;

    /** Write to a register with no data. */
    virtual bool write(uint8_t reg) = 0;

    /** Acquire the internal semaphore for this device.
     * take_nonblocking should be used from the timer process,
     * take_blocking from synchronous code (i.e. init) */
    virtual bool sem_take_nonblocking() = 0;
    virtual bool sem_take_blocking() = 0;

    /** Release the internal semaphore for this device. */
    virtual void sem_give() = 0;
};

/** SPI serial device. */
class AP_SerialBus_SPI : public AP_SerialBus
{
public:
    AP_SerialBus_SPI(enum AP_HAL::SPIDevice device, enum AP_HAL::SPIDeviceDriver::bus_speed speed);
    void init();
    uint16_t read_16bits(uint8_t reg);
    uint32_t read_24bits(uint8_t reg);
    uint32_t read_adc(uint8_t reg);
    bool write(uint8_t reg);
    bool sem_take_nonblocking();
    bool sem_take_blocking();
    void sem_give();

private:
    enum AP_HAL::SPIDevice _device;
    enum AP_HAL::SPIDeviceDriver::bus_speed _speed;
    AP_HAL::SPIDeviceDriver *_spi;
    AP_HAL::Semaphore *_spi_sem;
};

/** I2C serial device. */
class AP_SerialBus_I2C : public AP_SerialBus
{
public:
    AP_SerialBus_I2C(AP_HAL::I2CDriver *i2c, uint8_t addr);
    void init();
    uint16_t read_16bits(uint8_t reg);
    uint32_t read_24bits(uint8_t reg);
    bool write(uint8_t reg);
    bool sem_take_nonblocking();
    bool sem_take_blocking();
    void sem_give();

private:
    AP_HAL::I2CDriver *_i2c;
    uint8_t _addr;
    AP_HAL::Semaphore *_i2c_sem;
};

class AP_Baro_MS56XX : public AP_Baro_Backend
{
public:
    void update();
    void accumulate();

protected:
    AP_Baro_MS56XX(AP_Baro &baro, AP_SerialBus *serial, bool use_timer);
    void _init();

    virtual void _calculate() = 0;
    virtual bool _read_prom(uint16_t prom[8]);
    void _timer();

    AP_SerialBus *_serial;

    /* Asynchronous state: */
    volatile bool            _updated;
    volatile uint8_t         _d1_count;
    volatile uint8_t         _d2_count;
    volatile uint32_t        _s_D1, _s_D2;
    uint8_t                  _state;
    uint32_t                 _last_timer;

    bool _use_timer;

    // Internal calibration registers
    uint16_t                 _C1,_C2,_C3,_C4,_C5,_C6;
    float                    _D1,_D2;
    uint8_t _instance;
};

class AP_Baro_MS5611 : public AP_Baro_MS56XX
{
public:
    AP_Baro_MS5611(AP_Baro &baro, AP_SerialBus *serial, bool use_timer);
private:
    void _calculate();
};

class AP_Baro_MS5607 : public AP_Baro_MS56XX
{
public:
    AP_Baro_MS5607(AP_Baro &baro, AP_SerialBus *serial, bool use_timer);
private:
    void _calculate();
};

class AP_Baro_MS5637 : public AP_Baro_MS56XX
{
public:
    AP_Baro_MS5637(AP_Baro &baro, AP_SerialBus *serial, bool use_timer);
private:
    void _calculate();
    bool _read_prom(uint16_t prom[8]) override;
};
