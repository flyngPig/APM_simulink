/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

    -- Coded by Victor Mayoral Vilches --
*/

#include <AP_HAL/AP_HAL.h>
#if CONFIG_HAL_BOARD == HAL_BOARD_LINUX

#include "AP_InertialSensor_MPU9250.h"
#include <AP_HAL_Linux/GPIO.h>

#include <assert.h>

extern const AP_HAL::HAL& hal;

// MPU9250 accelerometer scaling for 16g range
#define MPU9250_ACCEL_SCALE_1G    (GRAVITY_MSS / 2048.0f)

#define MPUREG_XG_OFFS_TC                               0x00
#define MPUREG_YG_OFFS_TC                               0x01
#define MPUREG_ZG_OFFS_TC                               0x02
#define MPUREG_X_FINE_GAIN                              0x03
#define MPUREG_Y_FINE_GAIN                              0x04
#define MPUREG_Z_FINE_GAIN                              0x05

// MPU9250 registers
#define MPUREG_XA_OFFS_H                                0x77    // X axis accelerometer offset (high byte)
#define MPUREG_XA_OFFS_L                                0x78    // X axis accelerometer offset (low byte)
#define MPUREG_YA_OFFS_H                                0x7A    // Y axis accelerometer offset (high byte)
#define MPUREG_YA_OFFS_L                                0x0B    // Y axis accelerometer offset (low byte)
#define MPUREG_ZA_OFFS_H                                0x0D    // Z axis accelerometer offset (high byte)
#define MPUREG_ZA_OFFS_L                                0x0E    // Z axis accelerometer offset (low byte)

// MPU6000 & MPU9250 registers
// not sure if present in MPU9250
// #define MPUREG_PRODUCT_ID                               0x0C    // Product ID Register
#define MPUREG_XG_OFFS_USRH                     0x13    // X axis gyro offset (high byte)
#define MPUREG_XG_OFFS_USRL                     0x14    // X axis gyro offset (low byte)
#define MPUREG_YG_OFFS_USRH                     0x15    // Y axis gyro offset (high byte)
#define MPUREG_YG_OFFS_USRL                     0x16    // Y axis gyro offset (low byte)
#define MPUREG_ZG_OFFS_USRH                     0x17    // Z axis gyro offset (high byte)
#define MPUREG_ZG_OFFS_USRL                     0x18    // Z axis gyro offset (low byte)
#define MPUREG_SMPLRT_DIV                               0x19    // sample rate.  Fsample= 1Khz/(<this value>+1) = 200Hz
#       define MPUREG_SMPLRT_1000HZ                             0x00
#       define MPUREG_SMPLRT_500HZ                              0x01
#       define MPUREG_SMPLRT_250HZ                              0x03
#       define MPUREG_SMPLRT_200HZ                              0x04
#       define MPUREG_SMPLRT_100HZ                              0x09
#       define MPUREG_SMPLRT_50HZ                               0x13
#define MPUREG_CONFIG                                           0x1A
#define MPUREG_GYRO_CONFIG                                      0x1B
// bit definitions for MPUREG_GYRO_CONFIG
#       define BITS_GYRO_FS_250DPS                              0x00
#       define BITS_GYRO_FS_500DPS                              0x08
#       define BITS_GYRO_FS_1000DPS                             0x10
#       define BITS_GYRO_FS_2000DPS                             0x18
#       define BITS_GYRO_FS_MASK                                0x18    // only bits 3 and 4 are used for gyro full scale so use this to mask off other bits
#       define BITS_GYRO_ZGYRO_SELFTEST                 0x20
#       define BITS_GYRO_YGYRO_SELFTEST                 0x40
#       define BITS_GYRO_XGYRO_SELFTEST                 0x80
#define MPUREG_ACCEL_CONFIG                             0x1C
#define MPUREG_MOT_THR                                  0x1F    // detection threshold for Motion interrupt generation.  Motion is detected when the absolute value of any of the accelerometer measurements exceeds this
#define MPUREG_MOT_DUR                                  0x20    // duration counter threshold for Motion interrupt generation. The duration counter ticks at 1 kHz, therefore MOT_DUR has a unit of 1 LSB = 1 ms
#define MPUREG_ZRMOT_THR                                0x21    // detection threshold for Zero Motion interrupt generation.
#define MPUREG_ZRMOT_DUR                                0x22    // duration counter threshold for Zero Motion interrupt generation. The duration counter ticks at 16 Hz, therefore ZRMOT_DUR has a unit of 1 LSB = 64 ms.
#define MPUREG_FIFO_EN                                  0x23
#define MPUREG_INT_PIN_CFG                              0x37
#       define BIT_INT_RD_CLEAR                                 0x10    // clear the interrupt when any read occurs
#       define BIT_LATCH_INT_EN                                 0x20    // latch data ready pin
#       define BIT_BYPASS_EN                                    0x02    // connect auxiliary I2C bus to the main I2C bus
#define MPUREG_INT_ENABLE                               0x38
// bit definitions for MPUREG_INT_ENABLE
#       define BIT_RAW_RDY_EN                                   0x01
#       define BIT_DMP_INT_EN                                   0x02    // enabling this bit (DMP_INT_EN) also enables RAW_RDY_EN it seems
#       define BIT_UNKNOWN_INT_EN                               0x04
#       define BIT_I2C_MST_INT_EN                               0x08
#       define BIT_FIFO_OFLOW_EN                                0x10
#       define BIT_ZMOT_EN                                              0x20
#       define BIT_MOT_EN                                               0x40
#       define BIT_FF_EN                                                0x80
#define MPUREG_INT_STATUS                               0x3A
// bit definitions for MPUREG_INT_STATUS (same bit pattern as above because this register shows what interrupt actually fired)
#       define BIT_RAW_RDY_INT                                  0x01
#       define BIT_DMP_INT                                              0x02
#       define BIT_UNKNOWN_INT                                  0x04
#       define BIT_I2C_MST_INT                                  0x08
#       define BIT_FIFO_OFLOW_INT                               0x10
#       define BIT_ZMOT_INT                                             0x20
#       define BIT_MOT_INT                                              0x40
#       define BIT_FF_INT                                               0x80
#define MPUREG_ACCEL_XOUT_H                             0x3B
#define MPUREG_ACCEL_XOUT_L                             0x3C
#define MPUREG_ACCEL_YOUT_H                             0x3D
#define MPUREG_ACCEL_YOUT_L                             0x3E
#define MPUREG_ACCEL_ZOUT_H                             0x3F
#define MPUREG_ACCEL_ZOUT_L                             0x40
#define MPUREG_TEMP_OUT_H                               0x41
#define MPUREG_TEMP_OUT_L                               0x42
#define MPUREG_GYRO_XOUT_H                              0x43
#define MPUREG_GYRO_XOUT_L                              0x44
#define MPUREG_GYRO_YOUT_H                              0x45
#define MPUREG_GYRO_YOUT_L                              0x46
#define MPUREG_GYRO_ZOUT_H                              0x47
#define MPUREG_GYRO_ZOUT_L                              0x48
#define MPUREG_USER_CTRL                                0x6A
// bit definitions for MPUREG_USER_CTRL
#       define BIT_USER_CTRL_SIG_COND_RESET             0x01            // resets signal paths and results registers for all sensors (gyros, accel, temp)
#       define BIT_USER_CTRL_I2C_MST_RESET              0x02            // reset I2C Master (only applicable if I2C_MST_EN bit is set)
#       define BIT_USER_CTRL_FIFO_RESET                 0x04            // Reset (i.e. clear) FIFO buffer
#       define BIT_USER_CTRL_DMP_RESET                  0x08            // Reset DMP
#       define BIT_USER_CTRL_I2C_IF_DIS                 0x10            // Disable primary I2C interface and enable hal.spi->interface
#       define BIT_USER_CTRL_I2C_MST_EN                 0x20            // Enable MPU to act as the I2C Master to external slave sensors
#       define BIT_USER_CTRL_FIFO_EN                    0x40            // Enable FIFO operations
#       define BIT_USER_CTRL_DMP_EN                             0x80            // Enable DMP operations
#define MPUREG_PWR_MGMT_1                               0x6B
#       define BIT_PWR_MGMT_1_CLK_INTERNAL              0x00            // clock set to internal 8Mhz oscillator
#       define BIT_PWR_MGMT_1_CLK_XGYRO                 0x01            // PLL with X axis gyroscope reference
#       define BIT_PWR_MGMT_1_CLK_YGYRO                 0x02            // PLL with Y axis gyroscope reference
#       define BIT_PWR_MGMT_1_CLK_ZGYRO                 0x03            // PLL with Z axis gyroscope reference
#       define BIT_PWR_MGMT_1_CLK_EXT32KHZ              0x04            // PLL with external 32.768kHz reference
#       define BIT_PWR_MGMT_1_CLK_EXT19MHZ              0x05            // PLL with external 19.2MHz reference
#       define BIT_PWR_MGMT_1_CLK_STOP                  0x07            // Stops the clock and keeps the timing generator in reset
#       define BIT_PWR_MGMT_1_TEMP_DIS                  0x08            // disable temperature sensor
#       define BIT_PWR_MGMT_1_CYCLE                             0x20            // put sensor into cycle mode.  cycles between sleep mode and waking up to take a single sample of data from active sensors at a rate determined by LP_WAKE_CTRL
#       define BIT_PWR_MGMT_1_SLEEP                             0x40            // put sensor into low power sleep mode
#       define BIT_PWR_MGMT_1_DEVICE_RESET              0x80            // reset entire device
#define MPUREG_PWR_MGMT_2                               0x6C            // allows the user to configure the frequency of wake-ups in Accelerometer Only Low Power Mode
#define MPUREG_BANK_SEL                                 0x6D            // DMP bank selection register (used to indirectly access DMP registers)
#define MPUREG_MEM_START_ADDR                   0x6E            // DMP memory start address (used to indirectly write to dmp memory)
#define MPUREG_MEM_R_W                                  0x6F            // DMP related register
#define MPUREG_DMP_CFG_1                                0x70            // DMP related register
#define MPUREG_DMP_CFG_2                                0x71            // DMP related register
#define MPUREG_FIFO_COUNTH                              0x72
#define MPUREG_FIFO_COUNTL                              0x73
#define MPUREG_FIFO_R_W                                 0x74
#define MPUREG_WHOAMI                                   0x75
#define MPUREG_WHOAMI_MPU9250                           0x71
#define MPUREG_WHOAMI_MPU9255                           0x73

/* bit definitions for MPUREG_MST_CTRL */
#define MPUREG_I2C_MST_CTRL                             0x24
#        define I2C_MST_P_NSR                           0x10
#        define I2C_SLV0_EN                             0x80
#        define I2C_MST_CLOCK_400KHZ                    0x0D
#        define I2C_MST_CLOCK_258KHZ                    0x08
#define MPUREG_I2C_SLV4_CTRL                            0x34
#define MPUREG_I2C_MST_DELAY_CTRL                       0x67
#        define I2C_SLV0_DLY_EN                         0x01
#        define I2C_SLV1_DLY_EN                         0x02
#        define I2C_SLV2_DLY_EN                         0x04
#        define I2C_SLV3_DLY_EN                         0x08
#define READ_FLAG                                       0x80
#define MPUREG_I2C_SLV0_ADDR                            0x25
#define MPUREG_EXT_SENS_DATA_00                         0x49
#define MPUREG_I2C_SLV0_DO                              0x63

// Configuration bits MPU 3000, MPU 6000 and MPU9250
#define BITS_DLPF_CFG_256HZ_NOLPF2              0x00
#define BITS_DLPF_CFG_188HZ                             0x01
#define BITS_DLPF_CFG_98HZ                              0x02
#define BITS_DLPF_CFG_42HZ                              0x03
#define BITS_DLPF_CFG_20HZ                              0x04
#define BITS_DLPF_CFG_10HZ                              0x05
#define BITS_DLPF_CFG_5HZ                               0x06
#define BITS_DLPF_CFG_2100HZ_NOLPF              0x07
#define BITS_DLPF_CFG_MASK                              0x07

#define DEFAULT_SMPLRT_DIV MPUREG_SMPLRT_1000HZ
#define DEFAULT_SAMPLE_RATE (1000 / (DEFAULT_SMPLRT_DIV + 1))

/*
 *  PS-MPU-9250A-00.pdf, page 8, lists LSB sensitivity of
 *  gyro as 16.4 LSB/DPS at scale factor of +/- 2000dps (FS_SEL==3)
 */
#define GYRO_SCALE (0.0174532f / 16.4f)

/*
 *  PS-MPU-9250A-00.pdf, page 9, lists LSB sensitivity of
 *  accel as 4096 LSB/mg at scale factor of +/- 8g (AFS_SEL==2)
 *
 *  See note below about accel scaling of engineering sample MPUXk
 *  variants however
 */

/*
 * 2 bytes for each in this order: ACC_X, ACC_Y, ACC_Z, TEMP, GYRO_X, GYRO_Y
 * and GYRO_Z
 */
#define MPU9250_SAMPLE_SIZE 14

/* SPI bus driver implementation */
AP_MPU9250_BusDriver_SPI::AP_MPU9250_BusDriver_SPI(AP_HAL::SPIDeviceDriver *spi)
{
    _spi = spi;
}

void AP_MPU9250_BusDriver_SPI::init()
{
    // disable I2C as recommended by the datasheet
    write8(MPUREG_USER_CTRL, BIT_USER_CTRL_I2C_IF_DIS);
}

void AP_MPU9250_BusDriver_SPI::read8(uint8_t reg, uint8_t *val)
{
    uint8_t addr = reg | 0x80; // Set most significant bit

    uint8_t tx[2];
    uint8_t rx[2];

    tx[0] = addr;
    tx[1] = 0;
    _spi->transaction(tx, rx, 2);

    *val = rx[1];
}

void AP_MPU9250_BusDriver_SPI::read_block(uint8_t reg, uint8_t *val, uint8_t count)
{
    assert(count < 32);

    uint8_t addr = reg | 0x80; // Set most significant bit
    uint8_t tx[32] = { addr, };
    uint8_t rx[32];

    _spi->transaction(tx, rx, count + 1);
    memcpy(val, rx + 1, count);
}

void AP_MPU9250_BusDriver_SPI::write8(uint8_t reg, uint8_t val)
{
    uint8_t tx[2];
    uint8_t rx[2];

    tx[0] = reg;
    tx[1] = val;
    _spi->transaction(tx, rx, 2);
}

void AP_MPU9250_BusDriver_SPI::set_bus_speed(AP_HAL::SPIDeviceDriver::bus_speed speed)
{
    _spi->set_bus_speed(speed);
}

bool AP_MPU9250_BusDriver_SPI::read_data_transaction(uint8_t *samples, uint8_t &n_samples)
{
    /* one register address followed by seven 2-byte registers */
    struct PACKED {
        uint8_t cmd;
        uint8_t int_status;
        uint8_t v[MPU9250_SAMPLE_SIZE];
    } rx, tx = { cmd : MPUREG_INT_STATUS | 0x80, };

    _spi->transaction((const uint8_t *)&tx, (uint8_t *)&rx, sizeof(rx));

    if (!(rx.int_status & BIT_RAW_RDY_INT)) {
        n_samples = 0;
#if MPU9250_DEBUG
        hal.console->printf("MPU9250: No sample available.\n");
#endif
        return false;
    }

    n_samples = 1;
    memcpy(&samples[0], &rx.v[0], MPU9250_SAMPLE_SIZE);

    return true;
}

AP_HAL::Semaphore* AP_MPU9250_BusDriver_SPI::get_semaphore()
{
    return _spi->get_semaphore();
}

bool AP_MPU9250_BusDriver_SPI::has_auxiliary_bus()
{
    return true;
}

/* I2C bus driver implementation */
AP_MPU9250_BusDriver_I2C::AP_MPU9250_BusDriver_I2C(AP_HAL::I2CDriver *i2c, uint8_t addr)
    : _addr(addr)
    , _i2c(i2c)
{
}

void AP_MPU9250_BusDriver_I2C::init()
{
    uint8_t value;

    read8(MPUREG_INT_PIN_CFG, &value);
    // enable I2C bypass, connecting auxiliary I2C bus to the main one
    value |= BIT_BYPASS_EN;
    write8(MPUREG_INT_PIN_CFG, value);
}

void AP_MPU9250_BusDriver_I2C::read8(uint8_t reg, uint8_t *val)
{
    _i2c->readRegister(_addr, reg, val);
}

void AP_MPU9250_BusDriver_I2C::read_block(uint8_t reg, uint8_t *val, uint8_t count)
{
    _i2c->readRegisters(_addr, reg, count, val);
}

void AP_MPU9250_BusDriver_I2C::write8(uint8_t reg, uint8_t val)
{
    _i2c->writeRegister(_addr, reg, val);
}

bool AP_MPU9250_BusDriver_I2C::read_data_transaction(uint8_t *samples,
                                                     uint8_t &n_samples)
{
    uint8_t ret = 0;
    struct PACKED {
        uint8_t int_status;
        uint8_t v[MPU9250_SAMPLE_SIZE];
    } buffer;

    ret = _i2c->readRegisters(_addr, MPUREG_INT_STATUS, sizeof(buffer), (uint8_t *)&buffer);
    if (ret != 0) {
        hal.console->printf("MPU9250: error in I2C read\n");
        n_samples = 0;
        return false;
    }

    if (!(buffer.int_status & BIT_RAW_RDY_INT)) {
#if MPU9250_DEBUG
        hal.console->printf("MPU9250: No sample available.\n");
#endif
        n_samples = 0;
        return false;
    }

    memcpy(samples, buffer.v, MPU9250_SAMPLE_SIZE);
    n_samples = 1;
    return true;
}

AP_HAL::Semaphore* AP_MPU9250_BusDriver_I2C::get_semaphore()
{
    return _i2c->get_semaphore();
}

bool AP_MPU9250_BusDriver_I2C::has_auxiliary_bus()
{
    return false;
}

AP_InertialSensor_MPU9250::AP_InertialSensor_MPU9250(AP_InertialSensor &imu, AP_MPU9250_BusDriver *bus) :
	AP_InertialSensor_Backend(imu),
    _bus(bus),
#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_PXF
    _default_rotation(ROTATION_ROLL_180_YAW_270)
#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_NAVIO
    /* no rotation needed */
    _default_rotation(ROTATION_NONE)
#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_ERLEBRAIN2
    _default_rotation(ROTATION_YAW_270)
#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_BBBMINI
    _default_rotation(ROTATION_NONE)
#else /* rotate for bbone default (and other boards) */
    _default_rotation(ROTATION_ROLL_180_YAW_90)
#endif
{
}

/*
  detect the sensor
 */
AP_InertialSensor_Backend *AP_InertialSensor_MPU9250::detect(AP_InertialSensor &_imu, AP_HAL::SPIDeviceDriver *spi)
{
    AP_MPU9250_BusDriver *bus = new AP_MPU9250_BusDriver_SPI(spi);
    if (!bus)
        return NULL;
    return _detect(_imu, bus, HAL_INS_MPU9250);
}

AP_InertialSensor_Backend *AP_InertialSensor_MPU9250::detect_i2c(AP_InertialSensor &_imu,
                                                                 AP_HAL::I2CDriver *i2c,
                                                                 uint8_t addr)
{
    AP_MPU9250_BusDriver *bus = new AP_MPU9250_BusDriver_I2C(i2c, addr);
    if (!bus)
        return nullptr;
    return _detect(_imu, bus, HAL_INS_MPU9250);
}

/* Common detection method - it takes ownership of the bus, freeing it if it's
 * not possible to return an AP_InertialSensor_Backend */
AP_InertialSensor_Backend *AP_InertialSensor_MPU9250::_detect(AP_InertialSensor &_imu,
                                                              AP_MPU9250_BusDriver *bus,
                                                              int16_t id)
{
    AP_InertialSensor_MPU9250 *sensor = new AP_InertialSensor_MPU9250(_imu, bus);
    if (sensor == NULL) {
        delete bus;
        return NULL;
    }
    if (!sensor->_init_sensor()) {
        delete sensor;
        delete bus;
        return NULL;
    }

    sensor->_id = id;

    return sensor;
}

/*
  initialise the sensor
 */
bool AP_InertialSensor_MPU9250::_init_sensor()
{
    _bus_sem = _bus->get_semaphore();

    if (!_hardware_init())
        return false;

    _gyro_instance = _imu.register_gyro(DEFAULT_SAMPLE_RATE);
    _accel_instance = _imu.register_accel(DEFAULT_SAMPLE_RATE);

    _product_id = AP_PRODUCT_ID_MPU9250;

    // start the timer process to read samples
    hal.scheduler->register_timer_process(FUNCTOR_BIND_MEMBER(&AP_InertialSensor_MPU9250::_poll_data, void));

#if MPU9250_DEBUG
    _dump_registers();
#endif
    return true;
}

/*
  update the accel and gyro vectors
 */
bool AP_InertialSensor_MPU9250::update( void )
{
    update_gyro(_gyro_instance);
    update_accel(_accel_instance);

    return true;
}

/*================ HARDWARE FUNCTIONS ==================== */

/**
 * Timer process to poll for new data from the MPU9250.
 */
void AP_InertialSensor_MPU9250::_poll_data(void)
{
    if (!_bus_sem->take_nonblocking()) {
        /*
          the semaphore being busy is an expected condition when the
          mainline code is calling wait_for_sample() which will
          grab the semaphore. We return now and rely on the mainline
          code grabbing the latest sample.
        */
        return;
    }
    _read_data_transaction();
    _bus_sem->give();
}

/*
  read from the data registers and update filtered data
 */
void AP_InertialSensor_MPU9250::_read_data_transaction() 
{
    uint8_t n_samples;
    uint8_t rx[MPU9250_SAMPLE_SIZE];

    Vector3f accel, gyro;

    if (!_bus->read_data_transaction(rx, n_samples)) {
        return;
    }

#define int16_val(v, idx) ((int16_t)(((uint16_t)v[2*idx] << 8) | v[2*idx+1]))

    accel = Vector3f(int16_val(rx, 1),
                     int16_val(rx, 0),
                     -int16_val(rx, 2));
    accel *= MPU9250_ACCEL_SCALE_1G;
    accel.rotate(_default_rotation);
    _rotate_and_correct_accel(_accel_instance, accel);
    _notify_new_accel_raw_sample(_accel_instance, accel);

    gyro = Vector3f(int16_val(rx, 5),
                    int16_val(rx, 4),
                    -int16_val(rx, 6));
    gyro *= GYRO_SCALE;
    gyro.rotate(_default_rotation);

    _rotate_and_correct_gyro(_gyro_instance, gyro);
    _notify_new_gyro_raw_sample(_gyro_instance, gyro);
}

/*
  read an 8 bit register
 */
uint8_t AP_InertialSensor_MPU9250::_register_read(uint8_t reg)
{
    uint8_t val;
    _bus->read8(reg, &val);
    return val;
}

/*
  write an 8 bit register
 */
void AP_InertialSensor_MPU9250::_register_write(uint8_t reg, uint8_t val)
{
    _bus->write8(reg, val);
}

/*
  initialise the sensor configuration registers
 */
bool AP_InertialSensor_MPU9250::_hardware_init(void)
{
    // we need to suspend timers to prevent other SPI drivers grabbing
    // the bus while we do the long initialisation
    hal.scheduler->suspend_timer_procs();

    if (!_bus_sem->take(100)) {
        hal.console->printf("MPU9250: Unable to get semaphore");
        return false;
    }

    // initially run the bus at low speed
    _bus->set_bus_speed(AP_HAL::SPIDeviceDriver::SPI_SPEED_LOW);

    uint8_t value = _register_read(MPUREG_WHOAMI);
    if (value != MPUREG_WHOAMI_MPU9250 && value != MPUREG_WHOAMI_MPU9255) {
        hal.console->printf("MPU9250: unexpected WHOAMI 0x%x\n", (unsigned)value);
        goto fail_whoami;
    }

    // Chip reset
    uint8_t tries;
    for (tries = 0; tries < 5; tries++) {
        uint8_t user_ctrl = _register_read(MPUREG_USER_CTRL);

        /* First disable the master I2C to avoid hanging the slaves on the
         * auxiliary I2C bus */
        if (user_ctrl & BIT_USER_CTRL_I2C_MST_EN) {
            _register_write(MPUREG_USER_CTRL, user_ctrl & ~BIT_USER_CTRL_I2C_MST_EN);
            hal.scheduler->delay(10);
        }

        // reset device
        _register_write(MPUREG_PWR_MGMT_1, BIT_PWR_MGMT_1_DEVICE_RESET);
        hal.scheduler->delay(100);

        // bus-dependent initialization
        _bus->init();

        // Wake up device and select GyroZ clock. Note that the
        // MPU9250 starts up in sleep mode, and it can take some time
        // for it to come out of sleep
        _register_write(MPUREG_PWR_MGMT_1, BIT_PWR_MGMT_1_CLK_ZGYRO);
        hal.scheduler->delay(5);

        // check it has woken up
        if (_register_read(MPUREG_PWR_MGMT_1) == BIT_PWR_MGMT_1_CLK_ZGYRO) {
            break;
        }

        hal.scheduler->delay(10);
        uint8_t status = _register_read(MPUREG_INT_STATUS);
        if ((status & BIT_RAW_RDY_INT) != 0) {
            break;
        }
#if MPU9250_DEBUG
        _dump_registers();
#endif
    }

    if (tries == 5) {
        hal.console->println("Failed to boot MPU9250 5 times");
        goto fail_tries;
    }

    _register_write(MPUREG_PWR_MGMT_2, 0x00);            // only used for wake-up in accelerometer only low power mode

    // used no filter of 256Hz on the sensor, then filter using
    // the 2-pole software filter
    _register_write(MPUREG_CONFIG, BITS_DLPF_CFG_256HZ_NOLPF2);

    // set sample rate to 1kHz, and use the 2 pole filter to give the
    // desired rate
    _register_write(MPUREG_SMPLRT_DIV, DEFAULT_SMPLRT_DIV);
    _register_write(MPUREG_GYRO_CONFIG, BITS_GYRO_FS_2000DPS);  // Gyro scale 2000º/s

    // RM-MPU-9250A-00.pdf, pg. 15, select accel full scale 16g
    _register_write(MPUREG_ACCEL_CONFIG,3<<3);

    // configure interrupt to fire when new data arrives
    _register_write(MPUREG_INT_ENABLE, BIT_RAW_RDY_EN);

    // clear interrupt on any read, and hold the data ready pin high
    // until we clear the interrupt
    value = _register_read(MPUREG_INT_PIN_CFG);
    value |= BIT_INT_RD_CLEAR | BIT_LATCH_INT_EN;
    _register_write(MPUREG_INT_PIN_CFG, value);

    // now that we have initialized, we set the SPI bus speed to high
    _bus->set_bus_speed(AP_HAL::SPIDeviceDriver::SPI_SPEED_HIGH);

    _bus_sem->give();

    hal.scheduler->resume_timer_procs();

    return true;

fail_tries:
fail_whoami:
    _bus_sem->give();
    hal.scheduler->resume_timer_procs();
    _bus->set_bus_speed(AP_HAL::SPIDeviceDriver::SPI_SPEED_HIGH);
    return false;
}

AuxiliaryBus *AP_InertialSensor_MPU9250::get_auxiliary_bus()
{
    if (_auxiliar_bus)
        return _auxiliar_bus;

    if (_bus->has_auxiliary_bus())
        _auxiliar_bus = new AP_MPU9250_AuxiliaryBus(*this);

    return _auxiliar_bus;
}

#if MPU9250_DEBUG
// dump all config registers - used for debug
void AP_InertialSensor_MPU9250::_dump_registers(AP_MPU9250_BusDriver *bus)
{
    hal.console->println("MPU9250 registers");
    for (uint8_t reg=0; reg<=126; reg++) {
        uint8_t v = _register_read(bus, reg);
        hal.console->printf("%02x:%02x ", (unsigned)reg, (unsigned)v);
        if ((reg - (MPUREG_PRODUCT_ID-1)) % 16 == 0) {
            hal.console->println();
        }
    }
    hal.console->println();
}
#endif

AP_MPU9250_AuxiliaryBus::AP_MPU9250_AuxiliaryBus(AP_InertialSensor_MPU9250 &backend)
    : AuxiliaryBus(backend, 4)
{
}

AP_HAL::Semaphore *AP_MPU9250_AuxiliaryBus::get_semaphore()
{
    return AP_InertialSensor_MPU9250::from(_ins_backend)._bus_sem;
}

AuxiliaryBusSlave *AP_MPU9250_AuxiliaryBus::_instantiate_slave(uint8_t addr, uint8_t instance)
{
    /* Enable slaves on MPU9250 if this is the first time */
    if (_ext_sens_data == 0)
        _configure_slaves();

    return new AP_MPU9250_AuxiliaryBusSlave(*this, addr, instance);
}

void AP_MPU9250_AuxiliaryBus::_configure_slaves()
{
    AP_InertialSensor_MPU9250 &backend = AP_InertialSensor_MPU9250::from(_ins_backend);

    /* Enable the I2C master to slaves on the auxiliary I2C bus*/
    uint8_t user_ctrl = backend._register_read(MPUREG_USER_CTRL);
    backend._register_write(MPUREG_USER_CTRL, user_ctrl | BIT_USER_CTRL_I2C_MST_EN);

    /* stop condition between reads; clock at 400kHz */
    backend._register_write(MPUREG_I2C_MST_CTRL, I2C_MST_CLOCK_400KHZ | I2C_MST_P_NSR);

    /* Hard-code divider for internal sample rate, 1 kHz, resulting in a
     * sample rate of 100Hz */
    backend._register_write(MPUREG_I2C_SLV4_CTRL, 9);

    /* All slaves are subject to the sample rate */
    backend._register_write(MPUREG_I2C_MST_DELAY_CTRL, I2C_SLV0_DLY_EN
                            | I2C_SLV1_DLY_EN | I2C_SLV2_DLY_EN | I2C_SLV3_DLY_EN);
}

int AP_MPU9250_AuxiliaryBus::_configure_periodic_read(AuxiliaryBusSlave *slave,
                                                      uint8_t reg, uint8_t size)
{
    if (_ext_sens_data + size > MAX_EXT_SENS_DATA)
        return -1;

    AP_MPU9250_AuxiliaryBusSlave *mpu_slave =
        static_cast<AP_MPU9250_AuxiliaryBusSlave*>(slave);
    mpu_slave->_set_passthrough(reg, size);
    mpu_slave->_ext_sens_data = _ext_sens_data;
    _ext_sens_data += size;

    return 0;
}

AP_MPU9250_AuxiliaryBusSlave::AP_MPU9250_AuxiliaryBusSlave(AuxiliaryBus &bus, uint8_t addr,
                                                           uint8_t instance)
    : AuxiliaryBusSlave(bus, addr, instance)
    , _mpu9250_addr(MPUREG_I2C_SLV0_ADDR + _instance * 3)
    , _mpu9250_reg(_mpu9250_addr + 1)
    , _mpu9250_ctrl(_mpu9250_addr + 2)
    , _mpu9250_do(MPUREG_I2C_SLV0_DO + _instance)
{
}

int AP_MPU9250_AuxiliaryBusSlave::_set_passthrough(uint8_t reg, uint8_t size,
                                                   uint8_t *out)
{
    AP_InertialSensor_MPU9250 &backend = AP_InertialSensor_MPU9250::from(_bus.get_backend());
    uint8_t addr;

    /* Ensure the slave read/write is disabled before changing the registers */
    backend._register_write(_mpu9250_ctrl, 0);

    if (out) {
        backend._register_write(_mpu9250_do, *out);
        addr = _addr;
    } else {
        addr = _addr | READ_FLAG;
    }

    backend._register_write(_mpu9250_addr, addr);
    backend._register_write(_mpu9250_reg, reg);
    backend._register_write(_mpu9250_ctrl, I2C_SLV0_EN | size);

    return 0;
}

int AP_MPU9250_AuxiliaryBusSlave::passthrough_read(uint8_t reg, uint8_t *buf,
                                                   uint8_t size)
{
    assert(buf);

    if (_registered) {
        hal.console->println("Error: can't passthrough when slave is already configured");
        return -1;
    }

    int r = _set_passthrough(reg, size);
    if (r < 0)
        return r;

    /* wait the value to be read from the slave and read it back */
    hal.scheduler->delay(10);

    AP_InertialSensor_MPU9250 &backend = AP_InertialSensor_MPU9250::from(_bus.get_backend());
    backend._bus->read_block(MPUREG_EXT_SENS_DATA_00 + _ext_sens_data, buf, size);

    /* disable new reads */
    backend._register_write(_mpu9250_ctrl, 0);

    return size;
}

int AP_MPU9250_AuxiliaryBusSlave::passthrough_write(uint8_t reg, uint8_t val)
{
    if (_registered) {
        hal.console->println("Error: can't passthrough when slave is already configured");
        return -1;
    }

    int r = _set_passthrough(reg, 1, &val);
    if (r < 0)
        return r;

    /* wait the value to be written to the slave */
    hal.scheduler->delay(10);

    AP_InertialSensor_MPU9250 &backend = AP_InertialSensor_MPU9250::from(_bus.get_backend());

    /* disable new writes */
    backend._register_write(_mpu9250_ctrl, 0);

    return 0;
}

int AP_MPU9250_AuxiliaryBusSlave::read(uint8_t *buf)
{
    if (!_registered) {
        hal.console->println("Error: can't read before configuring slave");
        return -1;
    }

    AP_InertialSensor_MPU9250 &backend = AP_InertialSensor_MPU9250::from(_bus.get_backend());
    backend._bus->read_block(MPUREG_EXT_SENS_DATA_00 + _ext_sens_data, buf, _sample_size);

    return 0;
}

#endif // CONFIG_HAL_BOARD
