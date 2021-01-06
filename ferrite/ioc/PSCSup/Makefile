TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devPSC as a library for an IOC:
LIBRARY_IOC += devPSC

# Library Source files
devPSC_SRCS += \
	channel_rpmsg.cpp \
	channel_zmq.cpp \
	record_base.cpp \
	record_waveform.cpp \
	devPSC.cpp

# Link with the asyn and base libraries
devPSC_LIBS += $(EPICS_BASE_IOC_LIBS)

# Install .dbd and .db files
DBD += devPSC.dbd
DB_INSTALLS += $(TOP)/PSCSup/devPSC.db

#=======================================
include $(TOP)/configure/RULES
