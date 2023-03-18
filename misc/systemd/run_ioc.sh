#!/usr/bin/bash

export ARCH=linux-aarch64
export TOP=/opt/ioc
export LD_LIBRARY_PATH=/opt/epics_base/lib/$ARCH:/opt/ioc/lib/$ARCH

cd /opt/ioc/iocBoot/iocFer &&
/opt/ioc/bin/$ARCH/Fer st.cmd
