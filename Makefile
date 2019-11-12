all: inloop_sim
simulator: inloop_sim.o 
example_SC:	example_SC.o


INSTALL_PATH =
CC = icc
PEDANTIC_PARANOID_FREAK =       -O0 -Wshadow -Wcast-align \
                                -Waggregate-return -Wmissing-prototypes -Wmissing-declarations \
                                -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
                                -Wmissing-noreturn -Wredundant-decls -Wnested-externs \
                                -Wpointer-arith -Wwrite-strings -finline-functions
REASONABLY_CAREFUL_DUDE =       -Wall
NO_PRAYER_FOR_THE_WICKED =      -w -O2
WARNINGS =                      $(REASONABLY_CAREFUL_DUDE)
CFLAGS = -g $(WARNINGS)

INCLUDES = -I$(INSTALL_PATH)/include
DEFS = -L$(INSTALL_PATH)/lib/
LDADD = -lm -lsimgrid
LIBS =


%: %.o
	$(CC)  $(CFLAGS) $^ $(LIBS) $(LDADD) -o $@


%.o: %.c
	$(CC)  $(CFLAGS) -g -c -o $@ $<

clean:
	rm -f $(BIN_FILES) *.o *~
.SUFFIXES:
.PHONY: clean
