OUT=main
OBJS=main.o store.o table.o kvtable.o settable.o protocol.o cmd.o net.o reply.o dispatcher.o str.o

CFLAGS=-O3 -Wall -Wextra
LDFLAGS=-levent -pthread -lhaildb

all: $(OUT) Makefile

$(OUT): $(OBJS) Makefile
	$(CXX) $(LDFLAGS) -o $(OUT) $(OBJS)

%.o: %.cc %.h Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

%.o: %.cc Makefile
	$(CXX) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(OUT)

