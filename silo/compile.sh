#!/bin/bash
export PATH=`pwd`/..:$PATH
source ../run
MODE=perf CHECK_INVARIANTS=1 make -j
MODE=perf CHECK_INVARIANTS=1 make -j dbtest

