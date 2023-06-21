TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC_$(APP_ARCH) += Tornado
PROD_IOC = $(PROD_IOC_$(T_A))

# Tornado.dbd will be created and installed
DBD += Tornado.dbd

# Tornado.dbd will be made up from these files:
Tornado_DBD += base.dbd

# Tornado_registerRecordDeviceDriver.cpp derives from Tornado.dbd
Tornado_SRCS += Tornado_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
Tornado_SRCS_DEFAULT += TornadoMain.cpp

# Use Ferrite
Tornado_DBD += Ferrite.dbd
Tornado_LIBS += Ferrite
SHRLIB_SEARCH_DIRS += $(TOP)/target/debug

# Workaround to link shared library
ifeq ($(APP_ARCH),linux-x86_64)
	Tornado_LIBS += app app-plugin
else
	Tornado_LDFLAGS += -Wl,-Bdynamic -l:libapp.so -l:libapp-plugin.so
endif

# Finally link to the EPICS Base libraries
Tornado_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
