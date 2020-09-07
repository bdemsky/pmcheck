include ../benchmarks.mk

TESTNAME = ms-queue

HEADERS = my_queue.h
OBJECTS = main.o my_queue.o

CXXFLAGS += -I /scratch/fuzzer/random-fuzzer/include 

all: $(TESTNAME)

$(TESTNAME): $(HEADERS) $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS)

clean:
	rm -f $(TESTNAME) *.o
