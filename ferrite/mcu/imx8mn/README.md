# i.MX8M Nano M7 program

## Configure U-Boot (for imx8mn)

+ Load device tree `/boot/fsl-imx-var-som-m7.dtb`

  ```uboot
  env set fdt_file fsl-imx-var-som-m7.dtb
  ```

+ Enable Cortex-M7 core

  ```uboot
  env set use_m7 yes
  ```

+ Set adress to load program in TCM

  ```uboot
  env set m7_addr 0x7e0000
  ```

+ Set executable file name to `/boot/rpmsg_echo.bin`

  ```uboot
  env set m7_bin rpmsg_echo.bin
  ```

+ Save configuration

  ```uboot
  env save
  ```

+ Boot using `boot` command.

## Hints

+ Always call `NVIC_SetPriority` for interrupt. Otherwise accessing semaphores from the ISR will fail.
