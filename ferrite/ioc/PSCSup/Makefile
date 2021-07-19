TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devPSC as a library for an IOC:
LIBRARY_$(APP_ARCH) += devPSC
LIBRARY = $(LIBRARY_$(T_A))

# Library includes
devPSC_INCLUDES += -I$(APP_SRC_DIR)/src
aaiDevSupp_INCLUDES += -I$(APP_SRC_DIR)/src
aaoDevSupp_INCLUDES += -I$(APP_SRC_DIR)/src
mbbiDirectDevSupp_INCLUDES += -I$(APP_SRC_DIR)/src
mbboDirectDevSupp_INCLUDES += -I$(APP_SRC_DIR)/src

# Library Source files
devPSC_SRCS += \
	devPSC.cpp
devPSC_SRCS += aaiDevSupp.cpp
devPSC_SRCS += aaoDevSupp.cpp
devPSC_SRCS += mbbiDirectDevSupp.cpp
devPSC_SRCS += mbboDirectDevSupp.cpp

# Link with the libraries
devPSC_LIBS += $(EPICS_BASE_IOC_LIBS)

# Workaround to link shared library
ifeq ($(APP_FAKEDEV),)
	PSC_LDFLAGS += -Wl,-Bdynamic -l:libapp.so
else
	PSC_LIBS += app_fakedev
endif

# Install .dbd and .db files
DBD += devPSC.dbd

#=======================================
include $(TOP)/configure/RULES
