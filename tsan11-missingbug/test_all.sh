#!/bin/bash
set -e
set -u

for t in seqlock-test rwlock-test; do
  echo -n "$t " 
  ./test.sh ./$t
done

