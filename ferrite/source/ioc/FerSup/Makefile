TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devFer as a library for an IOC:
LIBRARY_$(APP_ARCH) += devFer
LIBRARY = $(LIBRARY_$(T_A))

# Library includes
INCLUDES += \
	-DUSE_TYPED_RSET

# Library Source files
devFer_SRCS += \
	devFer.c \
	_interface.c \
	_record.c \
	_array_record.c \
	ai.c \
	ao.c \
	aai.c \
	aao.c \
	waveform.c \
	bi.c \
	bo.c \
	mbbi_direct.c \
	mbbo_direct.c

# Link with the libraries
devFer_LIBS += $(EPICS_BASE_IOC_LIBS)

# Link app
include $(TOP)/link_app.mk

# Install .dbd and .db files
DBD += devFer.dbd

#=======================================
include $(TOP)/configure/RULES
