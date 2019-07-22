#!/bin/bash
source ../run
cd out-perf.check.masstree/
cd benchmarks/
./dbtest $@ -- --verbose -t 5
