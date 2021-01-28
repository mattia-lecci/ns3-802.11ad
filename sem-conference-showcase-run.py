# AUTHOR(S):
# Mattia Lecci <mattia.lecci@dei.unipd.it>
# 
# University of Padova (UNIPD), Italy
# Information Engineering Department (DEI) 
# SIGNET Research Group @ http://signet.dei.unipd.it/
# 
# Date: December 2020

import sem
import os
import sys
import argparse
from collections import OrderedDict
import sem_utils
import numpy as np
from matplotlib import pyplot as plt
import copy

sys.stdout.flush()


def run_simulations(applicationType, normOfferedTraffic, socketType, mpduAggregationSize,
                    phyMode, simulationTime, numStas, allocationPeriod,
                    accessCbapIfAllocated, biDurationUs, onoffPeriodMean,
                    onoffPeriodStdev, smartStart, numRuns):
    param_combination = OrderedDict({
        "applicationType": applicationType,
        "normOfferedTraffic": normOfferedTraffic,
        "socketType": socketType,
        "mpduAggregationSize": mpduAggregationSize,
        "phyMode": phyMode,
        "simulationTime": simulationTime,
        "numStas": numStas,
        "allocationPeriod": allocationPeriod,
        "accessCbapIfAllocated": accessCbapIfAllocated,
        "biDurationUs": biDurationUs,
        "onoffPeriodMean": onoffPeriodMean,
        "onoffPeriodStdev": onoffPeriodStdev,
        "smartStart": smartStart,
        "RngRun": list(range(numRuns)),
    })

    campaign.run_missing_simulations(param_combination)
    broken_results = campaign.get_results_as_numpy_array(param_combination,
                                                         check_stderr,
                                                         numRuns)
    # remove_simulations(broken_results)
    #
    # print("Run simulations with param_combination " + str(param_combination))
    # campaign.run_missing_simulations(
    #     param_combination
    # )

    param_combination.pop("RngRun")

    return param_combination


def remove_simulations(broken_results):
    print("Removing broken simulations")
    # TODO test map(campaign.db.delete_result, broken_results.flatten())
    for result in broken_results.flatten():
        if result:
            print("removing ", str(result))
            campaign.db.delete_result(result)
    # write updated database to disk
    campaign.db.write_to_disk()


def check_stderr(result):
    if len(result['output']['stderr']) > 0:
        print('Invalid simulation: ', result['meta']['id'], file=sys.stderr)
        return result
    else:
        return []


###############
# Main script #
###############
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--cores",
                        help="Value of sem.parallelrunner.MAX_PARALLEL_PROCESSES. Default: 1",
                        type=int,
                        default=1)
    parser.add_argument("--paramSet",
                        help="The parameter set of a given campaign. Available: {basic, onoff, onoff_stdev}. Mandatory parameter!",
                        default='')
    parser.add_argument("--numRuns",
                        help="The number of runs per simulation. Default: 5",
                        type=int,
                        default=5)
    parser.add_argument("--campaignName",
                        help="MANDATORY parameter for the campaign name. Suggested: commit hash",
                        default=None)
    # baseline parameters
    parser.add_argument("--applicationType",
                        help="The baseline applicationType. Default: onoff",
                        type=str,
                        default="onoff")
    parser.add_argument('--smartStartOn', dest='smartStart', action='store_true')
    parser.add_argument('--smartStartOff', dest='smartStart', action='store_false')
    parser.set_defaults(smartStart=True)
    parser.add_argument("--normOfferedTraffic",
                        help="The baseline normOfferedTraffic. Default: 0.75",
                        type=float,
                        default=0.75)
    parser.add_argument("--socketType",
                        help="The baseline socketType. Default: ns3::UdpSocketFactory",
                        type=str,
                        default="ns3::UdpSocketFactory")
    parser.add_argument("--mpduAggregationSize",
                        help="The baseline mpduAggregationSize [B]. Default: 262143",
                        type=int,
                        default=262143)
    parser.add_argument("--phyMode",
                        help="The baseline phyMode. Default: DMG_MCS4",
                        type=str,
                        default="DMG_MCS4")
    parser.add_argument("--simulationTime",
                        help="The baseline simulationTime [s]. Default: 10.0",
                        type=float,
                        default=10.0)
    parser.add_argument("--numStas",
                        help="The baseline numStas. Default: 4",
                        type=int,
                        default=4)
    parser.add_argument('--accessCbapIfAllocated', dest='accessCbapIfAllocated', action='store_true')
    parser.add_argument('--dontAccessCbapIfAllocated', dest='accessCbapIfAllocated', action='store_false')
    parser.set_defaults(accessCbapIfAllocated=True)
    parser.add_argument("--biDurationUs",
                        help="The baseline biDurationUs [us]. Default: 102400",
                        type=int,
                        default=102400)
    parser.add_argument("--onoffPeriodMean",
                        help="The baseline onoffPeriodMean [s]. Default: 0.1024",
                        type=float,
                        default=102.4e-3)
    parser.add_argument("--onoffPeriodStdev",
                        help="The baseline onoffPeriodStdev [s]. Default: 0.0",
                        type=float,
                        default=0.0)
    args = parser.parse_args()

    assert args.campaignName is not None, "Undefined parameter --campaignName"
    print(f'Starting sem simulation for paramSet={args.paramSet} with {args.cores} core(s)...')

    sem.parallelrunner.MAX_PARALLEL_PROCESSES = args.cores
    ns_path = os.path.dirname(os.path.realpath(__file__))
    campaign_name = args.campaignName
    script = "scheduler_comparison_qd_dense"
    campaign_dir = os.path.join(ns_path, "campaigns", "scheduler_comparison_qd_dense-" + campaign_name)

    # Set up campaign
    # skip_configuration parameter is not included in the official SEM release as of December 2020
    # It was, though, included in the develop branch https://github.com/signetlabdei/sem/tree/develop
    campaign = sem.CampaignManager.new(
        ns_path, script, campaign_dir,
        overwrite=False,
        runner_type="ParallelRunner",
        optimized=False,
        skip_configuration=True,
        check_repo=False
    )

    print("campaign: " + str(campaign))

    # Set up baseline parameters
    applicationType = args.applicationType
    smartStart = args.smartStart
    socketType = args.socketType
    mpduAggregationSize = args.mpduAggregationSize
    phyMode = args.phyMode
    simulationTime = args.simulationTime
    numStas = args.numStas
    allocationPeriod = [0, 1]  # 0: CbapOnly, n>0: BI/n
    accessCbapIfAllocated = args.accessCbapIfAllocated
    biDurationUs = args.biDurationUs
    onoffPeriodMean = args.onoffPeriodMean
    onoffPeriodStdev = args.onoffPeriodStdev
    normOfferedTraffic = args.normOfferedTraffic
    numRuns = args.numRuns

    if args.paramSet == 'basic':
        applicationType = "constant"
        allocationPeriod = [0, 1, 2, 3, 4]  # 0: CbapOnly, n>0: BI/n
        normOfferedTraffic = [0.01, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1]

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == 'onoff':
        applicationType = "onoff"
        allocationPeriod = [0, 1, 2, 3, 4]  # 0: CbapOnly, n>0: BI/n
        normOfferedTraffic = [0.01, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1]

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == 'onoff_stdev':
        applicationType = "onoff"
        allocationPeriod = [0, 1]  # 0: CbapOnly, n>0: BI/n
        onOffPeriodDeviationRatio = [0, 1e-3, 2e-3, 5e-3, 1e-2, 2e-2, 5e-2, 10e-2, 20e-2]
        onoffPeriodStdev = [r * onoffPeriodMean for r in onOffPeriodDeviationRatio]

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == 'spPeriodicity':
        applicationType = ["constant", "onoff", "crazyTaxi", "fourElements"]
        allocationPeriod = [0, 1, 2, 3, 4]  # 0: CbapOnly, n>0: BI/n
        onoffPeriodMean = 1 / 30  # 30 FPS
        onoffPeriodStdev = 0.1 * onoffPeriodMean

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == 'onoffPeriodicity':
        applicationType = "onoff"
        allocationPeriod = [0, 2]  # 0: CbapOnly, n>0: BI/n
        onoffPeriodRatio = [1, 1.75*0.5, 1.5*0.5, 1.25*0.5, 1.1*0.5, 0.5, 0.5/1.1, 0.5/1.25, 0.5/1.5, 0.5/1.75, 1/4]
        onoffPeriodMean = [r * biDurationUs/1e6 for r in onoffPeriodRatio]

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == "mcs":
        applicationType = "onoff"
        phyMode = [f"DMG_MCS{n}" for n in range(12 + 1)]  # MCS 0, ..., MCS 12

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == "numStas":
        applicationType = "onoff"
        numStas = [2, 4, 6, 8, 10]

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == "smartStart":
        applicationType = "onoff"
        smartStart = [True, False]

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)

    elif args.paramSet == "accessCbapIfAllocated":
        applicationType = "onoff"
        accessCbapIfAllocated = [True, False]

        param_combination = run_simulations(applicationType=applicationType,
                                            normOfferedTraffic=normOfferedTraffic,
                                            socketType=socketType,
                                            mpduAggregationSize=mpduAggregationSize,
                                            phyMode=phyMode,
                                            simulationTime=simulationTime,
                                            numStas=numStas,
                                            allocationPeriod=allocationPeriod,
                                            accessCbapIfAllocated=accessCbapIfAllocated,
                                            biDurationUs=biDurationUs,
                                            onoffPeriodMean=onoffPeriodMean,
                                            onoffPeriodStdev=onoffPeriodStdev,
                                            smartStart=smartStart,
                                            numRuns=numRuns)
    
    else:
        raise ValueError('paramsSet={} not recognized'.format(args.paramSet))
