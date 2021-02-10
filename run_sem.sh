#!/bin/bash

CORES=12
NUM_RUNS=3
CAMPAIGN_NAME=e3ab37

python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=burst_smartOn
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=burst_smartOff_cbapOn
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=burst_smartOff_cbapOff
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=burst_stdev_smartOn
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=burst_stdev_smartOff_cbapOn
python3 sem-conference-showcase.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --paramSet=burst_stdev_smartOff_cbapOff

python3 sem-conference-showcase-fixed-app-rate.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --appRate=50Mbps
python3 sem-conference-showcase-fixed-app-rate.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --appRate=100Mbps
python3 sem-conference-showcase-fixed-app-rate.py --cores=$CORES --numRuns=$NUM_RUNS --campaignName=$CAMPAIGN_NAME --appRate=200Mbps