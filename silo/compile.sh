#!/bin/bash
MODE=perf CHECK_INVARIANTS=1 make -j
MODE=perf CHECK_INVARIANTS=1 make -j dbtest

