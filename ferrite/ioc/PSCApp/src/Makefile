TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = PSC
# PSC.dbd will be created and installed
DBD += PSC.dbd

# PSC.dbd will be made up from these files:
PSC_DBD += base.dbd

# Include dbd files from all support applications:
PSC_DBD += devPSC.dbd

# Add all the support libraries needed by this IOC
PSC_LIBS += devPSC

# PSC_registerRecordDeviceDriver.cpp derives from PSC.dbd
PSC_SRCS += PSC_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
PSC_SRCS_DEFAULT += PSCMain.cpp
PSC_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#PSC_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
PSC_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

