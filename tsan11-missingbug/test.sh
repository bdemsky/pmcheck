#!/bin/bash

EXE=$1
TOTAL_RUN=1000
COUNT_ASSERT=0
COUNT_TIME=0

for i in `seq 1 1 $TOTAL_RUN` ; do
  OUTPUT="$(/usr/bin/time -f "time: %U %S" $EXE 2>&1)"
  ASSERT="$(echo "$OUTPUT" | grep "Assertion")"
  if [ -n "$ASSERT" ] ; then
    ((++COUNT_ASSERT))
  fi

  TIME="$(echo "$OUTPUT" | grep -o "time: .\... .\...")"
  TIME_USER_S="$(echo "$TIME" | cut -d' ' -f2 | cut -d'.' -f1)"
  TIME_USER_CS="$(echo "$TIME" | cut -d' ' -f2 | cut -d'.' -f2)"
  TIME_SYSTEM_S="$(echo "$TIME" | cut -d' ' -f3 | cut -d'.' -f1)"
  TIME_SYSTEM_CS="$(echo "$TIME" | cut -d' ' -f3 | cut -d'.' -f2)"

  TIME_EXE=$((10#$TIME_USER_S * 1000 + 10#$TIME_USER_CS * 10 + 10#$TIME_SYSTEM_S * 1000 + 10#$TIME_SYSTEM_CS * 10))
  COUNT_TIME=$((COUNT_TIME + TIME_EXE))
done

AVG_ASSERT=$(echo "${COUNT_ASSERT} * 100 / ${TOTAL_RUN}" | bc -l | xargs printf "%.1f")

# -3 / log(1 - p) < n
echo "Runs: $TOTAL_RUN | Assertions: $COUNT_ASSERT | Total time: ${COUNT_TIME}ms | Assert rate: ${AVG_ASSERT}%"
