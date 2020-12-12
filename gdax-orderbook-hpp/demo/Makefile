INCDIRS = -I ..
INCDIRS += -I ./dependencies/libcds-2.3.2
INCDIRS += -I ./dependencies/rapidjson-1.1.0/include
INCDIRS += -I ./dependencies/websocketpp-0.7.0

CDSLIBDIR = ./dependencies/libcds-2.3.2/build-release/bin

LIBS = -lboost_system # needed for boost::asio, used by websocketpp
                      # according to https://groups.google.com/forum/#!topic/websocketpp/Ou30S05FKoM
                      # one could use standalone Asio from think-async.com
LIBS += -lpthread
LIBS += -lssl # needed for websocketpp's TLS support (req'd by GDAX)
LIBS += -lcrypto # needed for websocketpp's TLS support (req'd by GDAX)
LIBS += -lcds

#test: demo
#	LD_LIBRARY_PATH=$(CDSLIBDIR) ./demo

#debug: demo
#	LD_LIBRARY_PATH=$(CDSLIBDIR) gdb ./demo

demo: demo.cpp ../gdax-orderbook.hpp \
      | dependencies/libcds-2.3.2/build-release/bin/libcds.so \
        dependencies/rapidjson-1.1.0 \
        dependencies/websocketpp-0.7.0
	../../clang++ demo.cpp -std=c++11 -o demo $(INCDIRS) $(LIBS) -L $(CDSLIBDIR)

dependencies/libcds-2.3.2/build-release/bin/libcds.so: \
		| dependencies/libcds-2.3.2
	cd dependencies/libcds-2.3.2 ; if [ ! -d build-release ]; then mkdir build-release; fi
	cd dependencies/libcds-2.3.2/build-release ; export PATH=$(PWD)/../..:/data/tsan11_build/cmake-3.6.2-Linux-x86_64/bin:$(PATH); CC=clang CXX=clang++ cmake -DCMAKE\_BUILD_TYPE=RELEASE ../
	cd dependencies/libcds-2.3.2/build-release ; export PATH=$(PWD)/../..:$(PATH); make -j4

clean:
	rm -rf demo dependencies/libcds-2.3.2/build-release
