# Buiding the project

### preparation
1. sudo apt-get install gawk

2. verify you have gcc-4.8
  - verify version using: sudo gcc --version
  - https://somewideopenspace.wordpress.com/2014/02/28/gcc-4-8-on-raspberry-pi-wheezy/

3. some times for cross platform building need to install aditional libreries:
  - sudo apt-get install g++-arm-linux-gnueabihf
  
4. only if building is failing
  - sudo apt-get install gawk

### clone
git clone https://github.com/flyngPig/APM_simulink.git

### making
1. cd to the quadCopter subfolder: cd /home/pi/ardupilot/ArduCopter
2. make -j4 navio
3. source: http://dev.ardupilot.com/wiki/building-for-navio-on-rpi2/


# runing

by default need to run in root mode (sudo)
- cd to the quadCopter subfolder: cd /home/pi/ardupilot/ArduCopter
- sudo ArduCopter ...

### prepare
1. configuring-i2c and SPI
  - https://learn.adafruit.com/adafruits-raspberry-pi-lesson-4-gpio-setup/configuring-i2c
  - http://www.emlid.com/navio-getting-started


# links

### add new mode
  - http://dev.ardupilot.com/wiki/apmcopter-adding-a-new-flight-mode/

### H/W
  - http://docs.emlid.com/navio/Navio-APM/hardware-setup-navio-plus/
  - http://copter.ardupilot.com/wiki/assembly-instructions/

