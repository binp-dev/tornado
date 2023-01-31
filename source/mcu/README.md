# i.MX8M Nano M7 program

## Configure U-Boot (for imx8mn)

+ Load device tree `/boot/imx8mn-var-som-symphony-m7.dtb`

  ```uboot
  env set fdt_file imx8mn-var-som-symphony-m7.dtb
  ```

+ Enable Cortex-M7 core

  ```uboot
  env set use_m7 yes
  ```

+ Set adress to load program in TCM

  ```uboot
  env set m7_addr 0x7e0000
  ```

+ Set executable file name to `/boot/m7image.bin`

  ```uboot
  env set m7_bin m7image.bin
  ```

+ Save configuration

  ```uboot
  env save
  ```

+ Boot using `bootcmd` command.

## Interrupts

+ Always call `NVIC_SetPriority` for interrupt. Otherwise accessing semaphores from the ISR will fail.
+ You cannot safely use the same interrupt controller along with Linux (without additional synchronization logic in kernel it causes the kernel to hang spontaneously).
+ To disable GPIO interrupt controller in kernel comment out `interrupts`, `interrupt-controller` and `#interrupt-cells` fields in corresponding `gpio*` section of device tree.

## Device tree building

You can build device tree without building the whole kernel by using the following commands:

```bash
gcc -E -P -x assembler-with-cpp -I include arch/arm64/boot/dts/freescale/imx8mn-var-som-symphony-m7.dts | \
dtc -I dts -O dtb -o arch/arm64/boot/dts/freescale/imx8mn-var-som-symphony-m7.dtb
```
