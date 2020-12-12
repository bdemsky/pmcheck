#!/bin/bash

EXE=$1
TOTAL_RUN=500
CDSLIB="/home/vagrant/c11tester"
export LD_LIBRARY_PATH=${CDSLIB}
export C11TESTER='-x1'

COUNT_DATA_RACE=0
COUNT_TIME=0

for i in `seq 1 1 $TOTAL_RUN` ; do
  OUTPUT="$( { time $EXE; } 2>&1 )"
  RACE="$(echo "$OUTPUT" | grep "race")"
  if [ -n "$RACE" ] ; then
    ((++COUNT_DATA_RACE))
  fi

  USER_TIME="$(echo "$OUTPUT" | grep -o "user..m.\....")"
  USER_TIME_S="$(echo $USER_TIME | cut -d 'm' -f2 | cut -d '.' -f1)"
  USER_TIME_MS="$(echo $USER_TIME | cut -d 'm' -f2 | cut -d '.' -f2)"

  SYS_TIME="$(echo "$OUTPUT" | grep -o "sys..m.\....")"
  SYS_TIME_S="$(echo $SYS_TIME | cut -d 'm' -f2 | cut -d '.' -f1)"
  SYS_TIME_MS="$(echo $SYS_TIME | cut -d 'm' -f2 | cut -d '.' -f2)"

  TIME_EXE=$((10#$USER_TIME_S * 1000 + 10#$USER_TIME_MS + 10#$SYS_TIME_S * 1000 + 10#$SYS_TIME_MS))
  COUNT_TIME=$((COUNT_TIME + TIME_EXE))
done

AVG_DATA_RACE=$(echo "${COUNT_DATA_RACE} * 100 / ${TOTAL_RUN}" | bc -l | xargs printf "%.1f")
AVG_TIME_INT=$(echo "${COUNT_TIME} / ${TOTAL_RUN} + 0.5" | bc -l | xargs printf "%.0f")

# -3 / log(1 - p) < n
#NO_99=$(echo "-3 / (l(1 - (${AVG_DATA_RACE} / 100)) / l(10)) + 0.5" | bc -l | xargs printf "%.0f")
#TIME_99=$(echo "${NO_99} * ${AVG_TIME_INT}" | bc -l)

echo "Runs: $TOTAL_RUN | Data races: $COUNT_DATA_RACE | Total time: ${COUNT_TIME}ms"
echo "Time: ${AVG_TIME_INT}ms | Race rate: ${AVG_DATA_RACE}%"
#echo "Time: ${AVG_TIME_INT}ms | Race rate: ${AVG_DATA_RACE}% | No. 99.9%: ${NO_99} | Time 99.9%: ${TIME_99}ms"
