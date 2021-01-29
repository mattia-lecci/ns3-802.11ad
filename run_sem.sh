#!/bin/bash

CORES=32
NUM_RUNS=10
CAMPAIGN_NAME=ffb435

python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=onoff
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=onoff_stdev
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=smartStart
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=mcs
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=numStas
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=basic
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=spPeriodicity
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=onoffPeriodicity
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=accessCbapIfAllocated
