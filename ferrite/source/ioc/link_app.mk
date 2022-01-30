# Workaround to link shared library
ifeq ($(APP_ARCH),linux-x86_64)
	PSC_LIBS += app
else
	PSC_LDFLAGS += -Wl,-Bdynamic -l:libapp.so
endif
