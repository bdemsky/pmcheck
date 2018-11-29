#!/bin/bash
cd examples
LD_LIBRARY_PATH=/usr/local/lib/ ./mb_insert_test
LD_LIBRARY_PATH=/usr/local/lib/ ./mb_longest_prefix_test
LD_LIBRARY_PATH=/usr/local/lib/ ./mb_lookup_test
LD_LIBRARY_PATH=/usr/local/lib/ ./mb_memory_only_test


