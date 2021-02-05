#!/bin/bash

CORES=12
NUM_RUNS=3
CAMPAIGN_NAME=e3ab37

python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=onoff_smartOn
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=onoff_smartOff
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=onoff_stdev
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=numStas
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=onoffPeriodicity
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=mcs
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=allApps

python3 sem-conference-showcase-fixed-app-rate.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --appRate=50Mbps
python3 sem-conference-showcase-fixed-app-rate.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --appRate=100Mbps
python3 sem-conference-showcase-fixed-app-rate.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --appRate=200Mbps