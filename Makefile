OUT=main
OBJS=main.o store.o table.o kvtable.o settable.o protocol.o
HAILDB=/opt/haildb

CFLAGS=-O0 -ggdb -I$(HAILDB)/include -Wall -Wextra
LDFLAGS=-levent -pthread -L$(HAILDB)/lib -lhaildb

all: $(OUT) Makefile

$(OUT): $(OBJS) Makefile
	$(CXX) $(LDFLAGS) -o $(OUT) $(OBJS)

%.o: %.cc %.h Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

%.o: %.cc Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(OUT)

