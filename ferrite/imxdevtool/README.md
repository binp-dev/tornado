# IMX-DEV-TOOL

Deployment and testing suite for controller software run on heterogenous i.MX* SoC family.

For project structure and usage examples see [the BINP next-gen power supply controller software](https://github.com/binp-automation/psc).

## Requirements

+ `gcc`
+ `g++`
+ `make`
+ `git`
+ `python3`

## Usage

The suite uses real device for testing. The device should be turned on and booted into Linux. The SSH access for `root` should be enabled without password prompt (e.g. using public key).

### Set environment variables

#### Components location

These variables specify the location of the following components. If they aren't set then the components will be downloaded to the default locations.

+ `EPICS_BASE` - EPICS base source. Default location: `epics-base`.
+ `ARMGCC_LINUX_DIR` - ARM GCC toolchain for Linux (`arm-linux-gnueabihf`). Default location: `armgcc/linux`.
+ `ARMGCC_DIR` - ARM GCC toolchain for MCU (`arm-none-eabi`). Default location: `armgcc/mcu`.
+ `FREERTOS_DIR` - FreeRTOS-Variscite source. Default location: `freertos-variscite`.

#### Target device location

+ `PSC_HOST` - IP address or hostname of device.

### Run testing process

```bash
python3 -m imxdevtool test --dev-addr <device-ip-address>
```
