# Workaround to link shared library
ifeq ($(APP_FAKEDEV),)
	PSC_LDFLAGS += -Wl,-Bdynamic -l:libapp.so
else
	PSC_LIBS += app_fakedev
endif
