#!/bin/bash
set -e
source /opt/intel/oneapi/setvars.sh
CONFIG_FILE="${1:-default.conf}"
OPTIMIZATION="${2:-O2}"
COUNT=1
if [[ ! -f "$CONFIG_FILE" ]]; then
		echo "Errore: il file '$CONFIG_FILE' non esiste."
		exit 1
fi
echo "Selected configuration file: $CONFIG_FILE"
for compiler in clang gcc icx; do
		for schedule in 1U 2U 3U; do
			echo "+=================================+"
			echo "|     Test $COUNT informations    |"
			echo "+=================================+"
			echo "|     Compiler: | $compiler       |"
			echo "|	    Schedule: | $schedule       |"
			echo "| Optimization: | -$OPTIMIZATION  |"
			echo "+=================================+"
			./build.sh "$CONFIG_FILE" OPT_LEVEL=-"$OPTIMIZATION" COMPILERNAME="$compiler" SCHEDULE_TYPE="$schedule"
			sudo "./build/$compiler/main"
			((COUNT += 1))
		done
done
DESTINATION="results/output_$OPTIMIZATION"
echo "Moving results inside: $DESTINATION"
sudo mkdir -p "$DESTINATION"
sudo mv output/ "$DESTINATION/"
echo "Tutti i test sono stati completati!"
