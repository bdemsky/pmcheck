#! /bin/sh

if [ -z $1 ] ; then
    echo "usage: $0 <dirname>"
elif [ -d $1 ] ; then
    echo "directory $1 already exists"
else
    autoconf2.13
    mkdir $1
    cd $1
    LLVM_ROOT="/scratch/llvm"
    CC="clang" \
    CXX="clang++" \
    CFLAGS="-Xclang -load -Xclang /scratch/llvm/build/lib/libCDSPass.so -L/scratch/fuzzer/random-fuzzer -lmodel" \
    CXXFLAGS="-Xclang -load -Xclang /scratch/llvm/build/lib/libCDSPass.so -L/scratch/fuzzer/random-fuzzer -lmodel" \
    LDFLAGS="-Xclang -load -Xclang /scratch/llvm/build/lib/libCDSPass.so -L/scratch/fuzzer/random-fuzzer -lmodel" \
            ../configure --disable-debug --enable-optimize="-O2 -gline-tables-only" --enable-llvm-hacks --disable-jemalloc
    make -j 8
fi
