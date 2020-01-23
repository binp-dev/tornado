DIR=$(INSTALL_LOCATION)/bin/$(shell $(EPICS_BASE)/startup/EpicsHostArch)
BIN=$(DIR)/$(NAME)_unittest

CFLAGS=\
	-DUNITTEST \
	-I.. \
	-I$(EPICS_BASE)/include \
	-I$(EPICS_BASE)/include/os/Linux \
	-I$(EPICS_BASE)/include/compiler/gcc

.PHONY: all test

FORCE: ;

all: $(BIN)

test: $(BIN)
	$(BIN)

$(BIN): main.cpp $(DIR) FORCE
	g++ $(CFLAGS) $< -o $@

$(DIR):
	mkdir -p $@
