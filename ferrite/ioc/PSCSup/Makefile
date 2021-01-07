TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devPSC as a library for an IOC:
LIBRARY_IOC += devPSC

# Library Source files
devPSC_SRCS += \
	record_base.cpp \
	record_waveform.cpp \
	framework.cpp \
	devPSC.cpp

ifndef $(USE_ZMQ)
	devPSC_SRCS += \
		channel_rpmsg.cpp
else
	devPSC_SRCS += \
		channel_zmq.cpp
endif

# Link with the asyn and base libraries
devPSC_LIBS += $(EPICS_BASE_IOC_LIBS)

# Install .dbd and .db files
DBD += devPSC.dbd
DB_INSTALLS += $(TOP)/PSCSup/devPSC.db

#=======================================
include $(TOP)/configure/RULES
