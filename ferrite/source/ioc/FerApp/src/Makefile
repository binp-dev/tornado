TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC_$(APP_ARCH) += Fer
PROD_IOC = $(PROD_IOC_$(T_A))

# Fer.dbd will be created and installed
DBD += Fer.dbd

# Fer.dbd will be made up from these files:
Fer_DBD += base.dbd

# Include dbd files from all support applications:
Fer_DBD += devFer.dbd

# Add all the support libraries needed by this IOC
Fer_LIBS += devFer

# Link app
include $(TOP)/link_app.mk

# Fer_registerRecordDeviceDriver.cpp derives from Fer.dbd
Fer_SRCS += Fer_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
Fer_SRCS_DEFAULT += FerMain.cpp
Fer_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#Fer_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
Fer_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

