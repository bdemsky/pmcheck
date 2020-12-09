#!/bin/bash

#MODE=perf CHECK_INVARIANTS=0 make -j
MODE=perf CHECK_INVARIANTS=0 USE_MALLOC_MODE=0 make -j dbtest 
