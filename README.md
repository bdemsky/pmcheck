C11Tester Concurrency Benchmarks
================================

This is a set of benchmarks that make use of C/C++11 atomic operations.

Getting Started
---------------

To get things working one has to come up with the following Build/Runtime Dependencies.
Corresponding command for Ubuntu is included for convenience.

1. GNU ncurses
-- sudo apt install libncurses5-dev libncursesw5-dev

2. GNU readline
-- sudo apt-get install libreadline6 libreadline6-dev

3. GLIBC
-- sudo apt-get install libc6

4. LLVM Compiler that supports C++11 w/o C11Tester instrumentation passes

5. packages for Silo

sudo apt-get install libdb++-dev
sudo apt-get install libaio-dev
sudo apt-get install libjemalloc-dev

6. Edit the scripts clang, clang++, g++, gcc, and run with the appropriate
paths for your system.

7.  Each benchmark contains a script compile.sh to build the benchmark
and run.sh to run the benchmark.