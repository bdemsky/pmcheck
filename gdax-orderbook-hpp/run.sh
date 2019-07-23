#!/bin/bash
source ../run
cd demo/
LD_LIBRARY_PATH=./dependencies/libcds-2.3.2/build-release/bin:$LD_LIBRARY_PATH ./demo $@


