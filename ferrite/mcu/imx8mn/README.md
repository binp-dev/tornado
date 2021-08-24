
## Configure U-Boot (for imx8mn)
* Load device tree `/boot/fsl-imx-var-som-m7.dtb`
  ```
  env set fdt_file fsl-imx-var-som-m7.dtb
  ```
* Enable Cortex-M7 core
  ```
  env set use_m7 yes
  ```
* Set adress to load program in TCM
  ```
  env set m7_addr 0x7e0000
  ```
* Set executable file name to `/boot/rpmsg_echo.bin`
  ```
  env set m7_bin rpmsg_echo.bin
  ```
* Save configuration
  ```
  env save
  ```
* Boot using `boot` command.
