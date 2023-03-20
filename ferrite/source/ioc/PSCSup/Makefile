TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devPSC as a library for an IOC:
LIBRARY_$(APP_ARCH) += devPSC
LIBRARY = $(LIBRARY_$(T_A))

# Library includes
INCLUDES += \
	-I$(APP_LIB_SRC)/src \
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
include $(TOP)/link_app.mk

# Install .dbd and .db files
DBD += devPSC.dbd

#=======================================
include $(TOP)/configure/RULES
