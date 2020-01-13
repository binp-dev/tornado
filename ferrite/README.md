# PSC-IOC

BINP next-gen power supply controller - Cortex-A7 Linux EPICS IOC

## How to build

### EPICS

#### Get EPICS base

```bash
git clone https://github.com/epics-base/epics-base.git
cd ./epics-base/
git checkout R7.0.3.1
git submodule update --init --recursive
```

#### Configure ARM cross-compilation

```diff
diff --git a/configure/CONFIG_SITE b/configure/CONFIG_SITE
index c46703f84..7070de79e 100644
--- a/configure/CONFIG_SITE
+++ b/configure/CONFIG_SITE
@@ -107,7 +107,7 @@
 # Which target architectures to cross-compile for.
 #  Definitions in configure/os/CONFIG_SITE.<host>.Common
 #  may override this setting.
-CROSS_COMPILER_TARGET_ARCHS=
+CROSS_COMPILER_TARGET_ARCHS=linux-arm
 #CROSS_COMPILER_TARGET_ARCHS=vxWorks-ppc32
 
 # If only some of your host architectures can compile the
diff --git a/configure/os/CONFIG_SITE.linux-x86.linux-arm b/configure/os/CONFIG_SITE.linux-x86.linux-arm
index 231c54bb8..663df740c 100644
--- a/configure/os/CONFIG_SITE.linux-x86.linux-arm
+++ b/configure/os/CONFIG_SITE.linux-x86.linux-arm
@@ -4,12 +4,12 @@
 #-------------------------------------------------------
 
 # Set GNU crosscompiler target name
-GNU_TARGET = arm-xilinx-linux-gnueabi
+GNU_TARGET = arm-linux-gnueabihf
 
 # Set GNU tools install path
 # Examples are installations at the APS:
 #GNU_DIR = /usr/local/vw/zynq-2011.09
-GNU_DIR = /usr/local/vw/zynq-2016.1/gnu/arm/lin
+GNU_DIR = /path/to/toolchain/gcc-linaro-6.3.1-2017.05-x86_64_arm-linux-gnueabihf
 #GNU_DIR = /usr/local/Xilinx/SDK/2016.3/gnu/arm/lin
 #GNU_DIR = /APSshare/XilinxSDK/2015.4/gnu/arm/lin
 
```

#### Build EPICS base

```bash
make clean uninstall
make
```

See also [EPICS installation instructions](https://epics.anl.gov/base/R7-0/3-docs/README.html).

### IOC

#### Set path to EPICS base we built previously

```diff
diff --git a/configure/RELEASE b/configure/RELEASE
index 9ddb97b..e15f212 100644
--- a/configure/RELEASE
+++ b/configure/RELEASE
@@ -30,7 +30,7 @@
 #SNCSEQ = $(MODULES)/seq-ver
 
 # EPICS_BASE should appear last so earlier modules can override stuff:
-EPICS_BASE = /opt/epics-base
+EPICS_BASE = /path/to/epics-base
 
 # Set RULES here if you want to use build rules from somewhere
 # other than EPICS_BASE:
```

#### Build IOC

```bash
make clean uninstall
make
```

## How to use

### Copy EPICS base and IOC to target device

Also you can specify `INSTALL_LOCATION` variable in `CONFIG_SITE` of EPICS base and IOC, and copy only that directory.

### Set environment variables

```bash
export EPICS_BASE=/path/to/epics-base
export EPICS_HOST_ARCH=$($EPICS_BASE/startup/EpicsHostArch)

export TOP=/path/to/psc-ioc

export PATH=$PATH:$EPICS_BASE/bin/$EPICS_HOST_ARCH/
export LD_LIBRARY_PATH=$EPICS_BASE/lib/$EPICS_HOST_ARCH:$TOP/lib/$EPICS_HOST_ARCH:$LD_LIBRARY_PATH
```

### Run IOC

```bash
cd $TOP/iocBoot/iocPSC/
../../bin/$EPICS_HOST_ARCH/PSC st.cmd
```
