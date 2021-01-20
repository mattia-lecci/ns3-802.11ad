#!/bin/bash

CORES=8
NUM_RUNS=1

python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=basic
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=onoff
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=onoff_stdev
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=spPeriodicity
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=onoffPeriodicity
