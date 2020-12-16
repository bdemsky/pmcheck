#!/bin/bash
set -e
set -u

echo "** Assertion test for broken data structures **"
for t in seqlock-test rwlock-test; do
  echo -n "$t " 
  ./test.sh ./$t
done

