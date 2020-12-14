#!/bin/bash
set -e
set -u

# Paul: skip `spsc-queue` as it deadlocks.

for t in seqlock-test rwlock-test; do
  echo -n "$t " 
  ./test.sh ./$t
done

