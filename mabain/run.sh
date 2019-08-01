#!/bin/bash
source ../run
cd examples
LD_LIBRARY_PATH=~/mabain/lib/:$LD_LIBRARY_PATH ./mb_rc_test $@
#LD_LIBRARY_PATH=~/mabain/lib/:$LD_LIBRARY_PATH ./mb_insert_test $@
#LD_LIBRARY_PATH=~/mabain/lib/:$LD_LIBRARY_PATH ./mb_longest_prefix_test $@
#LD_LIBRARY_PATH=~/mabain/lib/:$LD_LIBRARY_PATH ./mb_lookup_test $@
#LD_LIBRARY_PATH=~/mabain/lib/:$LD_LIBRARY_PATH ./mb_memory_only_test $@


