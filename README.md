# [WIP] EFM32 Firmware for surplus 3.4 GHz amplifier board
## Working
* Overcurrent protection (INA302 ALERT)
* Current monitoring
* Temperature measurement
* VDAC for gate bias supply (hard coded to around 70 mA Idq)

## TODO
* RX/TX sequencing
* Automatic Idq adjustment 
* Temperature warning / over temp shutdown
* Current warning

## Building / Flashing
For building the firmware arm-none-eabi-gcc is required. 
```shell
git clone --recurse-submodules https://github.com/dk2ro/9cm_efm32.git
cd 9cm_efm32
mkdir build && cd build
cmake ..
make -j 20
```

To upload the firmware to flash of the device, some kind of debug probe is required.
I had good success with using a Raspberry PI Pico board and the [yapicoprobe](https://github.com/rgrr/yapicoprobe) firmware. But any SWD Probe should work.

```shell
openocd -f interface/cmsis-dap.cfg -f target/efm32.cfg -c "reset_config srst_nogate connect_assert_srst" -c "program 9cm_efm32" -c reset -c shutdown
```

The ```-c "reset_config srst_nogate connect_assert_srst"``` is only necessary for flashing the first time, because otherwise the bootloader will interfere. On subsequent flashing, this option can be ommited. 

## Serial Output
Currently, the firmware will output the measured drain current and the two temperatures from the TMP432 (remote and local)
```
i_avg = 87 mA (min 85, max 90), temp_r = 22.6250, temp_l = 22.3125
```