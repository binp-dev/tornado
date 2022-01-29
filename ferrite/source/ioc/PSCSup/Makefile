TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devPSC as a library for an IOC:
LIBRARY_$(APP_ARCH) += devPSC
LIBRARY = $(LIBRARY_$(T_A))

# Library includes
INCLUDES += \
	-I$(APP_SRC_DIR)/src \
	-DUSE_TYPED_RSET

# Library Source files
devPSC_SRCS += \
	devPSC.cpp \
	base.cpp \
	iointr.cpp \
	waveform.cpp \
	aai.cpp \
	aao.cpp \
	mbbi_direct.cpp \
	mbbo_direct.cpp \
	ai.cpp \
	ao.cpp

# Link with the libraries
devPSC_LIBS += $(EPICS_BASE_IOC_LIBS)

# Link app
# Workaround to link shared library
ifeq ($(APP_ARCH),linux-x86_64)
	PSC_LIBS += app_fakedev
else
	PSC_LDFLAGS += -Wl,-Bdynamic -l:libapp.so
endif

# Install .dbd and .db files
DBD += devPSC.dbd

#=======================================
include $(TOP)/configure/RULES
