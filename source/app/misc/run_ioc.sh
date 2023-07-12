#!/usr/bin/bash

export ARCH=linux-aarch64
export TOP=/opt/ioc
export LD_LIBRARY_PATH=/opt/epics_base/lib/$ARCH:/opt/ioc/lib/$ARCH

source /opt/env.sh

cd /opt/ioc/iocBoot/iocTornado &&
/opt/ioc/bin/$ARCH/Tornado st.cmd
