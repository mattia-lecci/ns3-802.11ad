import matplotlib
import sem

matplotlib.use('Agg')
from os import path
from collections import OrderedDict


def get_results(result):
    """Extract simulation time"""
    return result['output']['results.csv']


###############
# Main script #
###############
if __name__ == '__main__':
    # Define campaign parameters
    ############################

    script = "evaluate_scheduler_qd_dense_scenario"
    campaign_name = "scheduler_comparison_v1"
    ns_path = path.dirname(path.realpath(__file__))
    campaign_dir = "./campaigns/" + campaign_name
    sem.parallelrunner.MAX_PARALLEL_PROCESSES = 32
    print("ns3-path=", ns_path)
    print("campaign-dir=", campaign_dir)

    # Create campaign
    #################

    # skip_configuration parameter is not included in the official SEM release as of December 2020
    # It was, though, included in the develop branch https://github.com/signetlabdei/sem/tree/develop
    campaign = sem.CampaignManager.new(ns_path, script, campaign_dir,
                                       runner_type="ParallelRunner",
                                       check_repo=False,
                                       skip_configuration=True,
                                       optimized=True,
                                       overwrite=False)

    print(campaign)

    # Parameter space
    #################

    verbose = False  # Enable DMG log components
    pcap = False  # Enable PCAP tracing
    nruns = 1  # Number of runs to perform for the same configuration of parameters
    numStas = 8  # Number of STAs to consider in the simulation [Max 10]
    applicationType = "onoff"  # The type of Application [onoff/bulk]
    socketType = "ns3::UdpSocketFactory"  # The type of Socket to use in the simulation [Tcp/Udp]
    tcpVariant = "NewReno"  # TCP variant for the simulation
    bufferSize = 131072  # TCP Send/Receive buffer size [bytes]
    packetSize = 1448  # Application payload size [bytes]
    dataRate = "100Mbps"  # Application-layer data rate
    mcs = "DMG_MCS5"  # MCS at the Physical layer
    queueSize = 1000  # Wifi MAC-layer queue size [packets]
    simulationTime = 15.0  # Simulation time [s]
    logComponentsStr = ":"  # Components to be logged from tLogStart to tLogEnd separated by ':'
    tLogStart = 0.0  # Log start [s]
    tLogEnd = simulationTime  # Log end [s]
    msduAggregation = 7935  # MSDU Aggregation size [bytes]
    mpduAggregationList = [0, 131072, 262143]  # MPDU Aggregation size [bytes]
    interAllocationList = [10]  # Duration of a broadcast CBAP between two ADDTS allocations [us]
    frameCapture = False  # True if a frame capture model is used
    frameCaptureMargin = 10  # Frame capture margin [dB]
    qdChannelFolder = "DenseScenario"  # The name of the folder containing the QD-Channel files
    schedulerTypeIdx = [0, 1, 2, 4, 6]  # Scheduling strategy at the PCP/AP

    param_combinations = OrderedDict({
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
        "schedulerTypeIdx": schedulerTypeIdx,
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
    print("Run simulations with param_combinations " + str(param_combinations))
    campaign.run_missing_simulations(param_combinations)

    print("Simulations done.")

    print("Extracting results...", flush=True)
    campaign.save_to_mat_file(param_combinations,
                              get_results,
                              'out.mat',
                              runs=nruns)
