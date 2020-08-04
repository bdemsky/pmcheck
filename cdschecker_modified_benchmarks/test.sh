#!/bin/bash

EXE=$1
TOTAL_RUN=5 #00
CDSLIB="/scratch/fuzzer/random-fuzzer"
export LD_LIBRARY_PATH=${CDSLIB}
export C11TESTER='-x1'

#ERROR_FILE="data-structure.log"
TASKSET=""

COUNT_DATA_RACE=0
COUNT_TIME=0

for i in `seq 1 1 $TOTAL_RUN` ; do
#  time ${TASKSET} $EXE &> $ERROR_FILE
#  OUTPUT=$(< $ERROR_FILE)

  OUTPUT="$(/usr/bin/time -f "time: %U %S" $EXE -x1 2>&1)"
  RACE="$(echo "$OUTPUT" | grep "race")"
  if [ -n "$RACE" ] ; then
    ((++COUNT_DATA_RACE))
  fi

  TIME="$(echo "$OUTPUT" | grep -o "time: .\... .\...")"
  TIME_USER_S="$(echo "$TIME" | cut -d' ' -f2 | cut -d'.' -f1)"
  TIME_USER_CS="$(echo "$TIME" | cut -d' ' -f2 | cut -d'.' -f2)"
  TIME_SYSTEM_S="$(echo "$TIME" | cut -d' ' -f3 | cut -d'.' -f1)"
  TIME_SYSTEM_CS="$(echo "$TIME" | cut -d' ' -f3 | cut -d'.' -f2)"

  TIME_EXE=$((10#$TIME_USER_S * 1000 + 10#$TIME_USER_CS * 10 + 10#$TIME_SYSTEM_S * 1000 + 10#$TIME_SYSTEM_CS * 10))
  COUNT_TIME=$((COUNT_TIME + TIME_EXE))
done

#rm $ERROR_FILE

AVG_DATA_RACE=$(echo "${COUNT_DATA_RACE} * 100 / ${TOTAL_RUN}" | bc -l | xargs printf "%.1f")
AVG_TIME_INT=$(echo "${COUNT_TIME} / ${TOTAL_RUN} + 0.5" | bc -l | xargs printf "%.0f")

# -3 / log(1 - p) < n
#NO_99=$(echo "-3 / (l(1 - (${AVG_DATA_RACE} / 100)) / l(10)) + 0.5" | bc -l | xargs printf "%.0f")
#TIME_99=$(echo "${NO_99} * ${AVG_TIME_INT}" | bc -l)

echo "Runs: $TOTAL_RUN | Data races: $COUNT_DATA_RACE | Total time: ${COUNT_TIME}ms"
echo "Time: ${AVG_TIME_INT}ms | Race rate: ${AVG_DATA_RACE}%"
#echo "Time: ${AVG_TIME_INT}ms | Race rate: ${AVG_DATA_RACE}% | No. 99.9%: ${NO_99} | Time 99.9%: ${TIME_99}ms"
