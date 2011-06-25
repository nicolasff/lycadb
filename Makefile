OUT=lycadb
TABLE_OBJS=src/table.o src/kvtable.o src/settable.o src/listtable.o src/zsettable.o
OBJS=src/lycadb.o src/store.o src/protocol.o src/cmd.o src/net.o src/reply.o src/dispatcher.o src/str.o src/config.o $(TABLE_OBJS)

CFLAGS=-O0 -ggdb -Wall -Wextra
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

