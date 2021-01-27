TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devPSC as a library for an IOC:
LIBRARY_IOC += devPSC

# Library includes
devPSC_INCLUDES += -I$(APP_SRC_DIR)/src

# Library Source files
devPSC_SRCS += \
	devPSC.cpp

# Link with the libraries
devPSC_LIBS += \
	$(EPICS_BASE_IOC_LIBS) \
	app_fakedev

#ifndef $(FAKEDEV)
#	devPSC_LIBS += app
#else
#	devPSC_LIBS += app_fakedev
#endif

# Install .dbd and .db files
DBD += devPSC.dbd
DB_INSTALLS += $(TOP)/PSCSup/devPSC.db

#=======================================
include $(TOP)/configure/RULES
