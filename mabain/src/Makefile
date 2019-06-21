CPP=clang++
TARGET=libmabain.so

all: $(TARGET)

<<<<<<< HEAD
CFLAGS  = -Xclang -load -Xclang /scratch/llvm-project/build/lib/libCDSPass.so -I. -I.. -Iutil -Wall -Werror -c -Wwrite-strings -Wsign-compare -Wcast-align -Wformat-security -fdiagnostics-show-option
CFLAGS += -g -ggdb -fPIC -O2 -std=c++11
=======
CFLAGS  = -Xclang -load -Xclang /scratch/llvm/build/lib/libCDSPass.so -I. -I.. -Iutil -Wall -Werror -c -Wwrite-strings -Wsign-compare -Wcast-align -Wformat-security -fdiagnostics-show-option
CFLAGS += -g -ggdb -fPIC -O1 -std=c++11
>>>>>>> e6990e6549277f6415536f7584bda1b584e194b7
CFLAGS += -D__SHM_LOCK__ -D__LOCK_FREE__ -D__SHM_QUEUE__
LDFLAGS = -pthread

SOURCES = $(wildcard *.cpp) $(wildcard util/*.cpp)
HEADERS = $(wildcard *.h) $(wildcard util/*.h)
OBJECTS = $(SOURCES:.cpp=.o)

$(TARGET):$(OBJECTS) $(HEADERS)
	$(CPP) -shared -o $(TARGET) $(OBJECTS) $(LDFLAGS)

.cpp.o: $(HEADERS) $(SOURCES)
	$(CPP) $(CFLAGS) $< -o $@

build: all

uninstall:
	-rm -rf $(INSTALLDIR)/include/mabain
	-rm -f $(INSTALLDIR)/lib/$(TARGET)
	-rm -f $(INSTALLDIR)/bin/mbc

clean:
	-rm -f *.o util/*.o $(TARGET)
	-make -C unittest clean

unit-test: all
	make -C unittest
