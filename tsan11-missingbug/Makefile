CC=../clang
CXX=../clang++

CXXFLAGS=-std=c++11 -pthread -Wall -g

SEQLOCK = seqlock-test
RWLOCK = rwlock-test

all: $(SEQLOCK) $(RWLOCK)

$(SEQLOCK): $(SEQLOCK).cc
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

$(RWLOCK): $(RWLOCK).cc
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(SEQLOCK) $(RWLOCK) *.o
