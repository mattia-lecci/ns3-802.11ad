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


def plot_line_metric(campaign, parameter_space, result_parsing_function, runs, xx, hue_var, xlabel, ylabel, filename, ylim=None, xscale="linear", yscale="linear"):
    print("Plotting (line): ", result_parsing_function.__name__)
    metric = campaign.get_results_as_xarray(parameter_space,
                                            result_parsing_function,
                                            xlabel,
                                            runs)
    # average over numRuns and squeeze
    metric_mean = metric.reduce(np.mean, 'runs').squeeze()
    metric_ci95 = metric.reduce(np.std, 'runs').squeeze() * 1.96 / np.sqrt(runs)

    fig = plt.figure()
    for val in metric_mean.coords[hue_var].values:
        plt.errorbar(xx, metric_mean.sel({hue_var: val}),
                     yerr=metric_ci95.sel({hue_var: val}),
                     label=f"{hue_var}={val}")
    plt.xscale(xscale)
    plt.yscale(yscale)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.ylim(ylim)
    plt.legend()
    plt.grid()
    fig.savefig(os.path.join(img_dir, filename))
    plt.close(fig)


def plot_bar_metric(campaign, parameter_space, result_parsing_function, out_labels, runs, xlabel, ylabel, filename):
    print("Plotting (bar): ", result_parsing_function.__name__)
    metric = campaign.get_results_as_xarray(parameter_space,
                                             result_parsing_function,
                                             out_labels,
                                             runs)

    # extract group variable
    metric_dims = list(metric.squeeze().dims)
    metric_dims.remove("metrics")  # multiple output from parsing function mapped into "metrics"
    if runs > 1:
        metric_dims.remove("runs")  # multiple runs are averaged
    assert len(metric_dims) == 1, f"There must only be one group_var, instead, metric_dims={metric_dims}"
    group_var = metric_dims[0]

    # extract dict from xarray: each key corresponds to a group, each element is the array related to the x-axis
    metric_mean_dict = {}
    metric_ci95_dict = {}
    for x in metric.coords[group_var].values:
        metric_mean_dict["{}={}".format(group_var, x)] = metric.sel({group_var: x}).reduce(np.mean, 'runs').squeeze()
        metric_ci95_dict["{}={}".format(group_var, x)] = metric.sel({group_var: x}).reduce(np.std, 'runs').squeeze() * 1.96 / np.sqrt(runs)

    fig, ax = plt.subplots()
    sem_utils.bar_plot(ax, metric_mean_dict, metric_ci95_dict)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_xticks(range(len(out_labels)))
    ax.set_xticklabels(out_labels)
    plt.grid()
    fig.savefig(os.path.join(img_dir, filename))
    plt.close(fig)


def plot_all_bars_metric(campaign, parameter_space, result_parsing_function, runs, for_each, ylabel, filename, alias_name=None, alias_vals=None):
        if alias_name is None:
            alias_name = for_each
            alias_vals = parameter_space[for_each]

        bar_plots_params = copy.deepcopy(parameter_space)
        for val, alias in zip(parameter_space[for_each], alias_vals):
            bar_plots_params[for_each] = [val]
            assert len(bar_plots_params['numStas']) == 1, "Cannot plot bar metric over list of numStas"
            
            folder_name = f"{alias_name}_{alias}_bars"
            os.makedirs(os.path.join(img_dir, folder_name), exist_ok=True)

            xtick_labels = ["AP"] + ["STA {}".format(i + 1) for i in range(bar_plots_params['numStas'][0])]

            plot_bar_metric(campaign,
                            bar_plots_params,
                            result_parsing_function,
                            xtick_labels,
                            runs,
                            xlabel="Node ID",
                            ylabel=ylabel,
                            filename=os.path.join(folder_name, filename))


def compute_avg_thr_mbps(pkts_df, dt):
    if len(pkts_df) > 0:
        rx_mb = pkts_df['PktSize_B'].sum() * 8 / 1e6
        thr_mbps = rx_mb / dt
    else:
        thr_mbps = 0

    return thr_mbps


def compute_avg_delay_ms(pkts_df):
    if len(pkts_df) > 0:
        delay = (pkts_df['RxTimestamp_ns'] - pkts_df['TxTimestamp_ns']).mean() / 1e9 * 1e3  # [ms]
    else:
        delay = np.nan

    return delay


def get_allocated_stas(result):
    sp_trace_df = sem_utils.output_to_df(result,
                                      data_filename="spTrace.csv",
                                      column_sep=',',
                                      numeric_cols='all')

    unique_stas = set(sp_trace_df['SrcNodeId'])
    # remove AP (0) and CBAP (255)
    unique_stas -= {0, 255}
    return list(unique_stas)


def compute_avg_user_metric(num_stas, pkts_df, metric):
    user_metric = [metric(pkts_df[pkts_df['SrcNodeId'] == srcNodeId])
                for srcNodeId in range(num_stas + 1)]

    return user_metric


def compute_norm_aggr_thr(result):
    pkts_df = sem_utils.output_to_df(result,
                                     data_filename="packetsTrace.csv",
                                     column_sep=',',
                                     numeric_cols='all')

    thr_mbps = compute_avg_thr_mbps(pkts_df, result['params']['simulationTime'])
    aggr_rate_mbps = result['params']['numStas'] * sem_utils.sta_data_rate_mbps(result['params']['numStas'],
                                                                                result['params']['phyMode'],
                                                                                result['params']['normOfferedTraffic'],
                                                                                result['params']['mpduAggregationSize'])
    norm_thr = thr_mbps / aggr_rate_mbps  # TODO check why sometimes >1
    return norm_thr


def compute_user_thr(result):
    pkts_df = sem_utils.output_to_df(result,
                                     data_filename="packetsTrace.csv",
                                     column_sep=',',
                                     numeric_cols='all')

    user_thr_mbps = compute_avg_user_metric(result['params']['numStas'], pkts_df, lambda df: compute_avg_thr_mbps(df, result['params']['simulationTime']))
    return user_thr_mbps


def compute_user_avg_delay(result):
    pkts_df = sem_utils.output_to_df(result,
                                     data_filename="packetsTrace.csv",
                                     column_sep=',',
                                     numeric_cols='all')

    user_delay_ms = compute_avg_user_metric(result['params']['numStas'], pkts_df, compute_avg_delay_ms)
    if np.any(np.isnan(user_delay_ms)):
        print(f"nan delay for {result['meta']['id']}", file=sys.stderr)
    return user_delay_ms


def compute_avg_aggr_delay_ms(result):
    pkts_df = sem_utils.output_to_df(result,
                                     data_filename="packetsTrace.csv",
                                     column_sep=',',
                                     numeric_cols='all')

    delay = compute_avg_delay_ms(pkts_df)
    if np.isnan(delay):
        print(f"no packets for {result['meta']['id']}", file=sys.stderr)
    return delay


def compute_std_aggr_delay_ms(result):
    pkts_df = sem_utils.output_to_df(result,
                                     data_filename="packetsTrace.csv",
                                     column_sep=',',
                                     numeric_cols='all')

    if len(pkts_df) > 0:
        delay_std_s = (pkts_df['RxTimestamp_ns'] - pkts_df['TxTimestamp_ns']).std() / 1e9 * 1e3  # [ms]
    else:
        delay_std_s = np.nan
        print(f"no packets for {result['meta']['id']}", file=sys.stderr)
    return delay_std_s


def compute_avg_delay_variation_ms(result):
    pkts_df = sem_utils.output_to_df(result,
                                     data_filename="packetsTrace.csv",
                                     column_sep=',',
                                     numeric_cols='all')

    if len(pkts_df) > 1:
        delay_s = (pkts_df['RxTimestamp_ns'] - pkts_df['TxTimestamp_ns']) / 1e9 * 1e3  # [ms]
        dv_s = np.mean(np.abs(np.diff(delay_s)))
    else:
        dv_s = np.nan
        print(f"no packets for {result['meta']['id']}", file=sys.stderr)
    return dv_s


def compute_jain_fairness(result):
    pkts_df = sem_utils.output_to_df(result,
                                     data_filename="packetsTrace.csv",
                                     column_sep=',',
                                     numeric_cols='all')

    user_thr = compute_avg_user_metric(result['params']['numStas'], pkts_df, lambda df: compute_avg_thr_mbps(df, result['params']['simulationTime']))
    
    if result['params']['allocationPeriod'] == 0:
        # fairness among all STAs
        # idx=0 is the AP and range() exlcudes last element
        allocated_stas = list(range(1, result['params']['numStas'] + 1))
    else:
          # fairness only among STAs with allocated SPs
        allocated_stas = get_allocated_stas(result)
    jain = sem_utils.jain_fairness([user_thr[i] for i in allocated_stas])
    return jain


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
    img_dir = os.path.join(campaign_dir, 'img', args.paramSet)

    # Set up campaign
    # skip_configuration parameter is not included in the official SEM release as of December 2020
    # It was, though, included in the develop branch https://github.com/signetlabdei/sem/tree/develop
    campaign = sem.CampaignManager.new(
        ns_path, script, campaign_dir,
        overwrite=False,
        runner_type="ParallelRunner",
        optimized=True,
        skip_configuration=True,
        check_repo=False
    )

    print("campaign: " + str(campaign))

    # Need to create the img/ folder after setting up a new campaign
    if not os.path.exists(img_dir):
        print("Making dir '{}'".format(img_dir))
        os.makedirs(img_dir)

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

    # Setup default plotting params
    line_plot_kwargs = dict()
    alias_name = None
    alias_vals = None

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

        # line plots vars
        xx = normOfferedTraffic
        hue_var = "allocationPeriod"
        xlabel = "Aggr. Offered Rate / PHY Rate"

        # bar plots var
        for_each = 'normOfferedTraffic'

    elif args.paramSet == 'onoff':
        applicationType = "onoff"
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

        # line plots vars
        xx = normOfferedTraffic
        hue_var = "allocationPeriod"
        xlabel = "Aggr. Offered Rate / PHY Rate"

        # bar plots var
        for_each = 'normOfferedTraffic'

    elif args.paramSet == 'onoff_stdev':
        applicationType = "onoff"
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

        # line plots vars
        xx = onOffPeriodDeviationRatio
        hue_var = "allocationPeriod"
        xlabel = "Period Deviation Ratio"
        line_plot_kwargs = {"xscale": "log"}

        # bar plots var
        for_each = 'onoffPeriodStdev'
        alias_name = 'onOffPeriodDeviationRatio'
        alias_vals = onOffPeriodDeviationRatio

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

        # line plots vars
        xx = allocationPeriod
        hue_var = "applicationType"
        xlabel = "Allocation period (BI/n, 0=CBAP)"

        # bar plots var
        for_each = 'applicationType'

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

        # line plots vars
        xx = onoffPeriodRatio
        hue_var = "allocationPeriod"
        xlabel = "OnOff App mean period (BI^-1)"

        # bar plots var
        for_each = 'allocationPeriod'

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

        # line plots vars
        xx = phyMode
        hue_var = "allocationPeriod"
        xlabel = "MCS"

        # bar plots var
        for_each = 'phyMode'

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

        # line plots vars
        xx = numStas
        hue_var = "allocationPeriod"
        xlabel = "numStas"
        
        # bar plots vars
        for_each = 'numStas'

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

        # line plots vars
        xx = smartStart
        hue_var = "allocationPeriod"
        xlabel = "smartStart"
        
        # bar plots vars
        for_each = 'allocationPeriod'

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

        # line plots vars
        xx = accessCbapIfAllocated
        hue_var = "allocationPeriod"
        xlabel = "accessCbapIfAllocated"
        
        # bar plots vars
        for_each = 'allocationPeriod'
    
    else:
        raise ValueError('paramsSet={} not recognized'.format(args.paramSet))


    # line plots
    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_norm_aggr_thr,
                     runs=numRuns,
                     xx=xx,
                     hue_var=hue_var,
                     xlabel=xlabel,
                     ylabel='Aggr. Throughput / Aggr. Offered Rate',
                     filename='thr.png',
                     **line_plot_kwargs)
    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_avg_aggr_delay_ms,
                     runs=numRuns,
                     xx=xx,
                     hue_var=hue_var,
                     xlabel=xlabel,
                     ylabel='Avg delay [ms]',
                     filename='avg_delay.png',
                     **line_plot_kwargs)
    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_avg_aggr_delay_ms,
                     runs=numRuns,
                     xx=xx,
                     hue_var=hue_var,
                     xlabel=xlabel,
                     ylabel='Avg delay [ms]',
                     filename='avg_delay_100ms.png',
                     ylim=(0, 100),
                     **line_plot_kwargs)
    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_std_aggr_delay_ms,
                     runs=numRuns,
                     xx=xx,
                     hue_var=hue_var,
                     xlabel=xlabel,
                     ylabel='Delay stdev [ms]',
                     filename='delay_stdev.png',
                     **line_plot_kwargs)
    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_avg_delay_variation_ms,
                     runs=numRuns,
                     xx=xx,
                     hue_var=hue_var,
                     xlabel=xlabel,
                     ylabel='Avg delay variation [ms]',
                     filename='avg_delay_variation.png',
                     **line_plot_kwargs)
    plot_line_metric(campaign=campaign,
                     parameter_space=param_combination,
                     result_parsing_function=compute_jain_fairness,
                     runs=numRuns,
                     xx=xx,
                     hue_var=hue_var,
                     xlabel=xlabel,
                     ylabel="Jain's Fairness Index",
                     filename='jain_fairness.png',
                     **line_plot_kwargs)

    # bar plots
    plot_all_bars_metric(campaign=campaign,
                        parameter_space=param_combination,
                        result_parsing_function=compute_user_thr,
                        runs=numRuns,
                        for_each=for_each,
                        ylabel="Throughput [Mbps]",
                        filename='user_thr.png',
                        alias_name=alias_name,
                        alias_vals=alias_vals)
    plot_all_bars_metric(campaign=campaign,
                        parameter_space=param_combination,
                        result_parsing_function=compute_user_avg_delay,
                        runs=numRuns,
                        for_each=for_each,
                        ylabel="Avg delay [ms]",
                        filename='user_delay.png',
                        alias_name=alias_name,
                        alias_vals=alias_vals)
