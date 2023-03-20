# Workaround to link shared library
ifeq ($(APP_ARCH),linux-x86_64)
	Fer_LIBS += app
else
	Fer_LDFLAGS += -Wl,-Bdynamic -l:libapp.so
endif
