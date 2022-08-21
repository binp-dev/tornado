TOP=..
include $(TOP)/configure/CONFIG
#=======================================

# Build devPSC as a library for an IOC:
LIBRARY_$(APP_ARCH) += devPSC
LIBRARY = $(LIBRARY_$(T_A))

# Library includes
INCLUDES += \
	-DUSE_TYPED_RSET
# -I$(CORE_SRC)/src

# Library Source files
devPSC_SRCS += \
	devPSC.c \
	_interface.c \
	_record.c \
	_array_record.c \
	ai.c \
	ao.c \
	aai.c \
	aao.c

# Link with the libraries
devPSC_LIBS += $(EPICS_BASE_IOC_LIBS)

# Link app
include $(TOP)/link_app.mk

# Install .dbd and .db files
DBD += devPSC.dbd

#=======================================
include $(TOP)/configure/RULES
