#!/bin/bash

CORES=32
NUM_RUNS=10

python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=basic
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=onoff
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=onoff_stdev
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=spPeriodicity
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=onoffPeriodicity
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=mcs
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=numStas
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=smartStart
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --paramSet=accessCbapIfAllocated
