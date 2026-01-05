# [WIP] EFM32 Firmware for surplus 3.4 GHz amplifier board
## Working
* Overcurrent protection (INA302 ALERT)
* Current monitoring
* Temperature measurement
* VDAC for gate bias supply
* Calibration routine for setting Idq

## TODO
* RX/TX sequencing
* $I_{Dq}$ temperature compensation 
* Temperature warning / over temp shutdown
* Current warning

## Hardware
Reverse engineering of the hardware was done by [eevblog.com forum members](https://www.eevblog.com/forum/rf-microwave/10w-rf-amplifier-3400-3800-mhz-from-aliexpress-reverse-engineering/)

This firmware uses the following pinout for 10 pin connector:

| Description |  Pin  | Pin | Description |
|------------:|:-----:|:---:|:------------|
|         5 V | **1** |  2  | 28 V        |
|         GND |   3   |  4  | UART TX     | 
|   TX Enable |   5   |  6  | UART RX     | 
|       SWDIO |   7   |  8  | SWDCLK      | 
|         GND |   9   | 10  | !RESET      | 


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

## Serial Output (115200 Baud)
Currently, the firmware will output the measured drain current and the two temperatures from the TMP432 (remote and local)
```
i_avg = 87 mA (min 85, max 90), temp_r = 22.6250, temp_l = 22.3125
```

## $I_{Dq}$ Calibration Routine
In order to achieve around 80 mA of drain quiescent current, this routine will tune the VDAC output value which biases the gate of the RF MOSFET.

To start the calibration routine, send the character 'c' over the serial connection.
