import sem
import math
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from os import path
from collections import OrderedDict

# Define campaign parameters
############################

script = "evaluate_scheduler_qd_dense_scenario"
campaign_name = "basic-vs-cbaponly-v2"        # define a campaign name in order to create different corresponding to a specific configuration
ns_path = path.dirname(path.realpath(__file__))
campaign_dir = "./campaigns/" + campaign_name
sem.parallelrunner.MAX_PARALLEL_PROCESSES = 32
print ("ns3-path=", ns_path)
print ("campaign-dir=", campaign_dir)

# Create campaign
#################

# skip_config parameter is not included in the SEM official release (check if the official release has been updated)
# You can find a SEM version with support to this parameter at https://github.com/tommyazz/sem
campaign = sem.CampaignManager.new(ns_path, script, campaign_dir,
    runner_type="ParallelRunner",
    check_repo=False,
    skip_config=True,
    optimized=False,
    overwrite=True)

print (campaign)

# Parameter space
#################

verbose = False                      # Enable DMG log components
pcap = False                         # Enable PCAP tracing 
nruns = 10                           # Number of runs to perform for the same configuration of parameters
numStas = 10                         # Number of STAs to consider in the simulation [Max 10]
applicationType = "onoff"            # The type of Application [onoff/bulk]
socketType = "ns3::UdpSocketFactory" # The type of Socket to use in the simulation [Tcp/Udp]
tcpVariant  = "NewReno"              # TCP variant for the simulation
bufferSize = 131072                  # TCP Send/Receive buffer size [bytes]
packetSize = 1448                    # Application payload size [bytes]
dataRate = "100Mbps"                 # Application-layer data rate 
mcs = "DMG_MCS4"                     # MCS at the Physical layer
queueSize = 1000                     # Wifi MAC-layer queue size [packets]
simulationTime = 10.0                # Simulation time [s]
logComponentsStr = ":"               # Components to be logged from tLogStart to tLogEnd separated by ':'
tLogStart = 0.0                      # Log start [s]
tLogEnd = simulationTime             # Log end [s]
msduAggregation = 7935               # MSDU Aggregation size [bytes]
mpduAggregationList = [0, 262143]    # MPDU Aggregation size [bytes]
interAllocationList = [10, 1500]     # Duration of a broadcast CBAP between two ADDTS allocations [us]
frameCapture = False                 # True if a frame capture model is used
frameCaptureMargin = 10              # Frame capture margin [dB]
qdChannelFolder = "DenseScenario"    # The name of the folder containing the QD-Channel files
schedulerList = ["ns3::CbapOnlyDmgWifiScheduler", "ns3::BasicDmgWifiScheduler"] # Scheduling strategy at the PCP/AP

param_combinations = OrderedDict ({
    "applicationType": applicationType,
    "packetSize": packetSize,
    "tcpVariant": tcpVariant,
    "socketType": socketType,
    "bufferSize": bufferSize,
    "msduAggregation": msduAggregation,
    "queueSize": queueSize,
    "frameCapture": frameCapture,
    "frameCaptureMargin": frameCaptureMargin,
    "verbose": verbose,
    "qdChannelFolder": qdChannelFolder,
    "numSTAs": numStas,
    "pcap": pcap,
    "simulationTime": simulationTime,
    "scheduler": schedulerList,
    "logComponentsStr": logComponentsStr,
    "tLogStart": tLogStart,
    "tLogEnd": tLogEnd,
    "mpduAggregation": mpduAggregationList,
    "interAllocation": interAllocationList,
    "dataRate": dataRate,
    "phyMode": mcs,
    "RngRun": list(range(nruns))
})

# Run simulations
#################
print("Run simulations with param_combination " + str(param_combinations))
campaign.run_missing_simulations (param_combinations)

print("Simulations done.")