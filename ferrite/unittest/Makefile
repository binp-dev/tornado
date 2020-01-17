BIN=../bin/linux-x86_64/unittest
CFLAGS=-I../PSCSup -I$(EPICS_BASE)/include -I$(EPICS_BASE)/include/os/Linux -I$(EPICS_BASE)/include/compiler/gcc

.PHONY: all test

all: $(BIN)

test: $(BIN)
	./$(BIN)

$(BIN): FORCE
	g++ $(CFLAGS) main.cpp -o $@

FORCE: ;
