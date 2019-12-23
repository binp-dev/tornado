# PSC-M4

BINP next-gen power supply controller - Cortex-M4 realtime part

## Usage

Load tools:

```bash
wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/5_4-2016q3/gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2
tar xvf gcc-arm-none-eabi-5_4-2016q3-20160926-linux.tar.bz2

git clone https://github.com/varigit/freertos-variscite.git -b freertos_bsp_1.0.1_imx7d-var01
```

Export variables:

```bash
export FREERTOS_DIR=/path/to/freertos-variscite
export ARMGCC_DIR=/path/to/gcc-arm-none-eabi
```

Build program image:

```bash
./build.sh
```

Upload image to device via `ssh`:

```bash
./deploy.sh <device-ip-address>
```

To configure U-Boot to run image at startup run the following U-Boot commands:

```uboot
setenv use_m4 yes
setenv m4image m4image.bin
setenv m4bootdata 0x007F8000
saveenv

run m4boot
run bootcmd
```

In Linux load `imx_rpmsg_tty` module:

```bash
modprobe imx_rpmsg_tty
```

Here is the Python example of sending and receiving messages via RPMsg:

```python3
import os, tty

fd = os.open("/dev/ttyRPMSG0", os.O_NOCTTY | os.O_RDWR)
tty.setraw(fd)

os.write(fd, b"test")
print(os.read(fd, 100))
```
